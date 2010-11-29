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
#ifdef DEVELOPER_MODE
	{ "local",	0, 0, 'l' },
#endif
	{ NULL,		0, 0, 0   },
};

static const char *optstr = "hnL:s:"
#ifdef DEVELOPER_MODE
	"l"
#endif
	;

static const char *optdesc[] = {
	"display this help text",
	"don't apply nastiness, just watch",
	"specify directory to be used for logging",
	"number of calls to skip before starting to intercept",
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

unsigned int verbosity = DBG_MASK;

/* IPC */
int inbound_fd;
int outbound_fd;

static struct child **children;	/* children array */
static int nchildren;		/* array size */
/*
 * maximum for the nchildren; since children can do all sorts
 * of setuid things, RLIMIT_NPROC is not really applicable here
 */
#define CHILDREN_MAX INT_MAX

static struct pollfd *fds;	/* main loop descriptors */

static void children2fds(void)
{
	int i;

	fds = realloc(fds, sizeof(*fds) * (nchildren + 1));
	fds[0].fd = inbound_fd;
	fds[0].events = POLLIN;

	for (i = 0; i < nchildren; i++) {
		fds[i + 1].fd = children[i]->fd;
		fds[i + 1].events = POLLIN | POLLHUP;
	}
}

struct child *child_new(pid_t pid, pid_t ppid)
{
	struct child *child;
	int fdsv[2], i;

	if (nchildren == CHILDREN_MAX) {
		error("too many child processes\n");
		/* XXX */
		return NULL;
	}

	/*fprintf(stderr, "### NULL: %d, 0x400000: %d\n",
		validate_addr(pid, 0, 0),
		validate_addr(pid, 0x400000, VA_OPT_READ | VA_OPT_EXEC));
	maps_save(pid, VA_OPT_OURS);*/

	child = xmalloc(sizeof(*child));
	child->pid = pid;
	child->ppid = ppid;

	i = socketpair(AF_UNIX, SOCK_STREAM, 0, fdsv);
	if (i) {
		error("Failed to create a pipe\n");
		exit(EXIT_FAILURE);
	}

	child->fd = fdsv[0];
	child->remote_fd = fdsv[1];
	child->state = CS_NEW;

	children = realloc(children, sizeof(struct child *) * ++nchildren);
	children[nchildren - 1] = child;

	children2fds();

	return child;
}

int child_find_idx_by_pid(pid_t pid)
{
	int i;

	for (i = 0; i < nchildren; i++) {
		if (children[i]->pid == pid)
			return i;
	}

	return -1;
}

struct child *child_find_by_pid(pid_t pid)
{
	int idx = child_find_idx_by_pid(pid);

	return idx >= 0 ? children[idx] : NULL;
}

void child_remove_by_idx(int idx)
{
	struct child **new_array;

	new_array = xmalloc((nchildren - 1) * sizeof(struct child *));
	memcpy(new_array, children, idx * sizeof(struct child *));
	memcpy(new_array + idx, children + idx + 1,
	       (nchildren - idx - 1) * sizeof(struct child *));

	free(children);
	children = new_array;
	nchildren--;
}

void child_free(struct child *child)
{
	if (!child) /* XXX: report */
		return;

	if (child->state != CS_EXITING)
		trace(DBG_CHILD, "child %d is in the wrong state %d\n",
		      child->pid, child->state);
	close(child->fd);
	close(child->remote_fd);
	free(child);
}

int children_wait(pid_t pid)
{
	int i, ret, mret = 0;

	for (i = 0; i < nchildren; i++)
		if (children[i]->pid == waitpid(children[i]->pid, &ret, WNOHANG)) {
			struct child *child = children[i];

			if (child->pid == pid)
				mret = WEXITSTATUS(ret);
			trace(DBG_CHILD, "child %d exited with %d code\n",
			      child->pid, WEXITSTATUS(ret));
			child_remove_by_idx(i);
			child_free(child);

			if (i < nchildren)
				continue;
		}

	return mret;
}

static void children_list(void)
{
	int i;
	return;
	trace(DBG_CHILD, "nchildren=%d\n", nchildren);
	for (i = 0; i < nchildren; i++)
		trace(DBG_CHILD, "[child %d: state %d]\n",
		      children[i]->pid, children[i]->state);
	trace(DBG_CHILD, "\n");
}

static int msg_handler_handshake(struct lrc_message *m, int fd)
{
	struct child *child;
	int vec[2], ret;
	struct msghdr msg = { 0 };
	struct cmsghdr *cmsg;
	struct iovec iov;
	pid_t dest_pid = m->pid;
	char buf[CMSG_SPACE(sizeof(int))];

	child = child_find_by_pid(m->pid);
	if (child)
		trace(0, "%d already exists\n", m->pid);
	else
		child = child_new(m->pid, 0);

	trace(DBG_PROTO, ">>> %d\n", m->pid);
	free(m);

	m = xmalloc(sizeof(*m) + sizeof(int) * 2);

	lrc_message_init(m, MT_RESPONSE);
	m->length = sizeof(int) * 2;
	vec[0] = child->fd;
	vec[1] = child->remote_fd;
	trace(0, "%d, %d\n", vec[0], vec[1]);
	memcpy(&m->payload.response.fds, vec, sizeof(vec));
	m->payload.response.code = RESP_OK;

	m->payload.response.code = 1; /* XXX */
	lrc_message_send(fd, m);
	free(m);

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = buf;
	msg.msg_controllen = CMSG_LEN(sizeof(int));
	iov.iov_base = &dest_pid;
	iov.iov_len = sizeof(dest_pid);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	*(int *)CMSG_DATA(cmsg) = vec[1];
	ret = sendmsg(fd, &msg, 0);
	trace(DBG_PROTO, "### sendmsg: %d: %m\n", ret);
	children2fds();

	return 0;
}

static int msg_handler_requestfd(struct lrc_message *m, int fd)
{
	struct child *child;
	int vec[2], ret;
	struct msghdr msg = { 0 };
	struct cmsghdr *cmsg;
	struct iovec iov;
	pid_t dest_pid = m->pid;
	char buf[CMSG_SPACE(sizeof(int))];

	child = child_find_by_pid(m->pid);

	trace(DBG_PROTO, ">>> %d\n", m->pid);
	free(m);

	m = xmalloc(sizeof(*m) + sizeof(int) * 2);

	lrc_message_init(m, MT_RESPONSE);
	m->length = sizeof(int) * 2;
	vec[0] = child->fd;
	vec[1] = child->remote_fd;
	trace(DBG_CHILD, "%d, %d\n", vec[0], vec[1]);
	memcpy(&m->payload.response.fds, vec, sizeof(vec));
	m->payload.response.code = RESP_OK;

	m->payload.response.code = 1; /* XXX */
	lrc_message_send(fd, m);
	free(m);

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = buf;
	msg.msg_controllen = CMSG_LEN(sizeof(int));
	iov.iov_base = &dest_pid;
	iov.iov_len = sizeof(dest_pid);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	*(int *)CMSG_DATA(cmsg) = vec[1];
	ret = sendmsg(fd, &msg, 0);
	trace(DBG_PROTO, "### sendmsg: %d: %m\n", ret);

	return 0;
}

static int msg_handler_fork(struct lrc_message *m, int fd)
{
	struct child *child;

	trace(DBG_CHILD, ">>> %d FORKED %d\n",
	      m->pid, m->payload.fork.child);

	child = child_find_by_pid(m->payload.fork.child);
	if (child)
		trace(DBG_CHILD, "%d already exists\n",
		      m->payload.fork.child);
	else
		child = child_new(m->payload.fork.child, 0);

	free(m);

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
	free(m);
	return 0;
}

static int msg_handler_exit(struct lrc_message *m, int fd)
{
	struct child *child;
	int i;

	trace(DBG_CHILD, "child %d is exiting with %d code\n",
	      m->pid, m->payload.exit.code);

	i = child_find_idx_by_pid(m->pid);
	child = children[i];

	child_remove_by_idx(i);
	child_free(child);
	free(m);

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
	[MT_REQUESTFD]	= msg_handler_requestfd,
	[MT_FORK]	= msg_handler_fork,
	[MT_LOGMSG]	= msg_handler_logmsg,
	[MT_EXIT]	= msg_handler_exit,
	[MT_BUG]	= msg_handler_bug,
};

/* main loop */
static int child_started(pid_t pid)
{
	int ret;
	struct lrc_message *m;

	trace(DBG_BASIC, "--- MAIN PID %d ---\n", pid);
	for (;;) {
		ret = xpoll(fds, nchildren + 1, !nchildren ? -1 : 0);
		if (ret > 0) {
			int i;
			for (i = 0; i < nchildren + 1 && ret; i++) {
				if (fds[i].revents & (POLLIN|POLLHUP|POLLNVAL))
					ret--;

				if (fds[i].revents & POLLNVAL) {
					struct child *child = children[i - 1];
					trace(DBG_PROTO, "ERROR: fd %d is closed\n",
					      fds[i].fd);

					child_remove_by_idx(i - 1);
					child_free(child);
					children2fds();
					continue;
				}

				if (fds[i].revents & POLLIN) {
					m = lrc_message_recv(fds[i].fd);
					(msg_handlers[m->type])(m, fds[i].fd);
				}

				if ((fds[i].revents & POLLHUP) && i > 0) {
					struct child *child = children[i - 1];

					trace(DBG_CHILD, "child %d hanged up\n",
					      child->pid);
					child_remove_by_idx(i - 1);
					child_free(child);
				}
			}
		}

		ret = children_wait(pid);

		children_list();
		if (!nchildren)
			break;
	}

	return ret;
}

static int spawn(const char *cmd, char *const argv[])
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

	return child_started(pid);
}

int main(int argc, char *const argv[])
{
	int loptidx, c, i;
	char **new_argv;
	char *executable;
	char *lrc_opts = NULL;
	char ipc_fdin[BUFSIZ];
	int no_crash = 0;
	char *logdir = NULL;
	unsigned long int skip_calls = 0;
	int svec[2];
	char buf[BUFSIZ];

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

	inbound_fd = svec[0];
	outbound_fd = svec[1];

	snprintf(ipc_fdin, BUFSIZ, "fd=%d:", outbound_fd);
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

	children2fds();

	/* perhaps it makes sense to fork and handle the exit codes */
	i = spawn(executable, new_argv);

	return i;
}
