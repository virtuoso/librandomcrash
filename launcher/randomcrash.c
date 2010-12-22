/*
 * librandomcrash program launcher
 *
 * Copyright (C) 2010 Alexander Shishkin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "util.h"
#include "proto.h"
#include "child.h"
#include "maps.h"

static const char my_name[] = "randomcrash";
static const char my_ver[] = VERSION;

static char lrc_path[PATH_MAX];

static const struct option options[] = {
	{ "help",	0, 0, 'h' },
	{ "no-crash",	0, 0, 'n' },
	{ "logdir",	1, 0, 'L' },
	{ "skip-calls",	1, 0, 's' },
	{ "verbosity",	1, 0, 'v' },
#ifdef DEVELOPER_MODE
	{ "local",	0, 0, 'l' },
#endif
	{ NULL,		0, 0, 0   },
};

static const char *optstr = "hnL:s:v:"
#ifdef DEVELOPER_MODE
	"l"
#endif
	;

static const char *optdesc[] = {
	"display this help text",
	"don't apply nastiness, just watch",
	"specify directory to be used for logging",
	"number of calls to skip before starting to intercept",
	"bitmask of the debug categories to be printed",
#ifdef DEVELOPER_MODE
	"run local build of the library instead",
#endif
};

static void usage(void)
{
	int i;

	fprintf(stderr, "%s (" PACKAGE " launcher), version %s\n\n",
		my_name, my_ver);

	for (i = 0; options[i].name; i++)
		fprintf(stderr, "-%c, --%s\n\t%s\n",
			options[i].val, options[i].name, optdesc[i]);
}

unsigned int verbosity;

/* IPC */
static int main_fd;

struct runqueue runners, waiters;

static struct pollfd *fds;	/* main loop descriptors */

static void runners2fds(void)
{
	int i;

	fds = realloc(fds, sizeof(*fds) * (runners.size + 1));
	fds[0].fd = main_fd;
	fds[0].events = POLLIN;

	for (i = 0; i < runners.size; i++) {
		fds[i + 1].fd = runners.array[i]->fd;
		fds[i + 1].events = POLLIN | POLLHUP;
	}
}

static void children_wait(void)
{
	int i, ret;

	for (i = 0; i < runners.size; i++)
		if (runners.array[i]->pid == waitpid(runners.array[i]->pid,
						     &ret, WNOHANG)) {
			struct child *child = runners.array[i];

			child->exit_code = WEXITSTATUS(ret);
			trace(DBG_CHILD, "child %d exited with %d code\n",
			      child->pid, child->exit_code);
			child_moveon_by_idx(i);

			if (i < runners.size)
				continue;
		}
}

/* --- LRC message handlers --- */
static int msg_handler_handshake(struct lrc_message *m, int fd)
{
	struct child *child;
	int ret;
	struct msghdr msg = { 0 };
	struct cmsghdr *cmsg;
	struct iovec iov;
	char buf[CMSG_SPACE(sizeof(int))];

	child = child_find_by_pid(&runners, m->pid);
	if (child)
		trace(DBG_CHILD, "%d already exists\n", m->pid);
	else
		child = child_new(m->pid, m->payload.handshake.ppid);

	trace(DBG_PROTO, ">>> %d\n", m->pid);
	xfree(m);

	m = xmalloc(sizeof(*m));

	/* our response packet is sent with sendmsg() instead */
	lrc_message_init(m, MT_RESPONSE);

	/* fd numbers are probably not needed */
	m->payload.response.fds[0] = child->fd;
	m->payload.response.fds[1] = child->remote_fd;
	m->payload.response.code = RESP_OK;
	m->payload.response.recipient = child->pid;

	lrc_message_prepare(m);

	/* ugh */
	memset(&msg, 0, sizeof(msg));
	msg.msg_control = buf;
	msg.msg_controllen = CMSG_LEN(sizeof(int));
	/* we include our message in the payload */
	iov.iov_base = m;
	iov.iov_len = sizeof(*m);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	*(int *)CMSG_DATA(cmsg) = child->remote_fd;

	ret = sendmsg(fd, &msg, 0);
	if (ret < 0)
		trace(DBG_PROTO, "sendmsg: %d: %m\n", ret);
	xfree(m);

	runners2fds();

	return 0;
}

static int msg_handler_imgood(struct lrc_message *m, int fd)
{
	struct child *child;

	child = child_find_by_pid(&runners, m->pid);

	trace(DBG_PROTO, "%d is good\n", m->pid);
	close(child->remote_fd);

	return 0;
}

static int msg_handler_fork(struct lrc_message *m, int fd)
{
	struct child *child;

	trace(DBG_CHILD, ">>> %d FORKED %d\n",
	      m->pid, m->payload.fork.child);

	child = child_find_by_pid(&runners, m->payload.fork.child);
	if (child)
		trace(DBG_CHILD, "%d already exists\n",
		      m->payload.fork.child);
	else {
		child = child_new(m->payload.fork.child, m->pid);
		runners2fds();
	}

	xfree(m);

	return 0;
}

static int msg_handler_logmsg(struct lrc_message *m, int fd)
{
	struct tm tm;

	localtime_r(&m->timestamp.tv_sec, &tm);
	log("[%d:%d@%02d:%02d:%02d.%ld] %s", m->pid,
	    m->payload.logmsg.level,
	    tm.tm_hour, tm.tm_min, tm.tm_sec, m->timestamp.tv_usec,
	    m->payload.logmsg.message);
	xfree(m);
	return 0;
}

static int msg_handler_exit(struct lrc_message *m, int fd)
{
	struct child *child;
	int i;

	trace(DBG_CHILD, "child %d is exiting with %d code\n",
	      m->pid, m->payload.exit.code);

	i = child_find_idx_by_pid(&runners, m->pid);
	if (i != -1)
		child_moveon_by_idx(i);

	i = child_find_idx_by_pid(&waiters, m->pid);
	if (i == -1 || i >= waiters.size) {
		error("can't find %d on either queue", m->pid);
		goto out;
	}

	child = waiters.array[i];
	child->exit_code = m->payload.exit.code;
	runners2fds();

out:
	xfree(m);

	return 0;
}

static int msg_handler_bug(struct lrc_message *m, int fd)
{
	trace(DBG_CHILD, "BUG in child %d\n", m->pid);
	exit(EXIT_FAILURE);
}

typedef int (*handlerfn)(struct lrc_message *, int);
static handlerfn msg_handlers[MT_NR_TOTAL] = {
	[MT_HANDSHAKE]	= msg_handler_handshake,
	[MT_IMGOOD]	= msg_handler_imgood,
	[MT_FORK]	= msg_handler_fork,
	[MT_LOGMSG]	= msg_handler_logmsg,
	[MT_EXIT]	= msg_handler_exit,
	[MT_BUG]	= msg_handler_bug,
};

/* main loop */
static void main_loop(void)
{
	int ret;
	struct lrc_message *m;

	for (;;) {
		ret = xpoll(fds, runners.size + 1, !runners.size ? -1 : 0);
		if (ret > 0) {
			int i;
			for (i = 0; i < runners.size + 1 && ret; i++) {
				if (fds[i].revents & (POLLIN|POLLHUP|POLLNVAL))
					ret--;

				if (fds[i].revents & POLLNVAL) {
					trace(DBG_PROTO, "ERROR: fd %d is closed\n",
					      fds[i].fd);

					child_moveon_by_idx(i - 1);
					runners2fds();
					continue;
				}

				if (fds[i].revents & POLLIN)
					m = lrc_message_recv(fds[i].fd);

				if ((fds[i].revents & POLLHUP) && i > 0) {
					struct child *child = runners.array[i - 1];

					trace(DBG_CHILD, "child %d hanged up\n",
					      child->pid);
					child_moveon_by_idx(i - 1);
				}

				if (fds[i].revents & POLLIN)
					(msg_handlers[m->type])(m, fds[i].fd);
			}
		}

		children_wait();

		/*runqueue_list(&runners);*/
		if (!runners.size)
			break;
	}
}

static pid_t spawn(const char *cmd, char *const argv[])
{
	pid_t pid;
	int ret;

	pid = fork();
	if (!pid) {
		ret = execve(cmd, argv, environ);
		if (ret) {
			fprintf(stderr, "exec %s failed: %m\n", cmd);
			exit(EXIT_FAILURE);
		}
	} else if (pid < 0) {
		fprintf(stderr, "fork %s failed: %m\n", cmd);
		exit(EXIT_FAILURE);
	}

	return pid;
}

int main(int argc, char *const argv[])
{
	unsigned long int skip_calls = 0;
	char *lrc_opts = NULL;
	char ipc_fdin[BUFSIZ];
	char *logdir = NULL;
	struct child *ours;
	int loptidx, c, i;
	char *executable;
	int no_crash = 0;
	char buf[BUFSIZ];
	char **new_argv;
	int svec[2];
	pid_t pid;

#ifdef DEVELOPER_MODE
	signal(SIGABRT, sigabrt_dumper);
#endif

	snprintf(lrc_path, PATH_MAX, "%s/lib/librandomcrash.so.%s",
		 PKG_PREFIX, my_ver);
	for (;;) {
		c = getopt_long(argc, argv, optstr, options, &loptidx);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				usage();
				exit(EXIT_SUCCESS);

			case 'n':
				no_crash++;
				break;

			case 'L':
				logdir = xstrdup(optarg);
				break;

			case 's':
				skip_calls = strtoul(optarg, NULL, 0);
				break;

			case 'v':
				verbosity = strtoul(optarg, NULL, 0);
				break;

#ifdef DEVELOPER_MODE
			case 'l':
				snprintf(lrc_path, PATH_MAX,
					 "%s/src/.libs/librandomcrash.so.%s",
					 LRC_SRC_BASE_DIR, my_ver);
				break;
#endif
			default:
				usage();
				exit(EXIT_FAILURE);
		}
	}

	if (argc == optind) {
		fprintf(stderr, "What do you want to %s today?\n\n", my_name);
		usage();
		exit(EXIT_FAILURE);
	}

	executable = argv[optind++];
	new_argv = xmalloc((argc - optind + 2) * sizeof(char *));
	new_argv[0] = executable;

	for (i = 1; optind < argc; i++, optind++)
		new_argv[i] = xstrdup(argv[optind]);
	new_argv[i] = NULL;

	setenv("LD_PRELOAD", lrc_path, 1);

	/* IPC init */
	i = socketpair(AF_UNIX, SOCK_STREAM, 0, svec);
	if (i) {
		fprintf(stderr, "Failed to create a pipe\n");
		exit(EXIT_FAILURE);
	}

	main_fd = svec[0];

	snprintf(ipc_fdin, BUFSIZ, "fd=%d:", svec[1]);
	lrc_opts = append_string(lrc_opts, ipc_fdin);

	if (no_crash)
		lrc_opts = append_string(lrc_opts, "no-crash:");

	if (logdir)
		lrc_opts = append_strings(lrc_opts, 3, "logdir=", logdir, ":");

	if (skip_calls) {
		snprintf(buf, BUFSIZ, "%lu", skip_calls);
		lrc_opts = append_strings(lrc_opts, 3, "skip-calls=", buf, ":");
	}

	setenv(LRC_CONFIG_ENV, lrc_opts ? lrc_opts : "", 1);

	runners2fds();

	pid = spawn(executable, new_argv);
	main_loop();

	ours = child_find_by_pid(&waiters, pid);
	i = ours->exit_code;

	runqueue_clean(&runners);
	runqueue_clean(&waiters);

	return i;
}
