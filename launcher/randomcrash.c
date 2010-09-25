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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

#include "proto.h"

static const char my_name[] = "randomcrash";
static const char my_ver[] = VERSION;

static char lrc_path[PATH_MAX];

#ifdef __LRC_DEBUG__
#define DEVELOPER_MODE

#include "../src/backtrace.c"

void sigabrt_dumper(int sig)
{
	int i;

	fprintf(stderr, "signal %d received\n", sig);
	lrc_dump_trace(2);
}
#endif

#ifndef LRC_CONFIG_ENV
#define LRC_CONFIG_ENV "LRC_CONFIG"
#endif

#ifndef LRC_SRC_BASE_DIR
#define LRC_SRC_BASE_DIR get_current_dir_name()
#endif

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

#ifdef DEVELOPER_MODE
#define trace(n, fmt, args ...) fprintf(stderr, "> " #n " <: " fmt, ## args)
#else
#define trace(n, fmt, args ...)
#endif

static void usage(void)
{
	int i;

	fprintf(stderr, "%s (" PACKAGE " launcher), version %s\n\n",
		my_name, my_ver);

	for (i = 0; options[i].name; i++)
		fprintf(stderr, "-%c, --%s\n\t%s\n",
			options[i].val, options[i].name, optdesc[i]);
}

static inline void *xmalloc(size_t len)
{
	void *x = malloc(len);

	if (!x) {
		fprintf(stderr, "We're short on memory\n");
		exit(EXIT_FAILURE);
	}

	return x;
}

static inline char *xstrdup(const char *str)
{
	void *x = strdup(str);

	if (!x) {
		fprintf(stderr, "We're short on memory\n");
		exit(EXIT_FAILURE);
	}

	return x;
}

static char *append_string(char *string, char *data)
{
	char *s = string;
	int sz, len;

	sz = string ? strlen(string) : 0;
	len = strlen(data);
	s = realloc(s, sz + len + 1);
	if (!s)
		exit(EXIT_FAILURE);
	memcpy(s + sz, data, len);
	s[sz + len] = 0;

	return s;
}

static char *append_strings(char *string, int n, ...)
{
	va_list ap;
	int i;
	char *s = string;

	va_start(ap, n);
	for (i = 0; i < n; i++)
		s = append_string(s, va_arg(ap, char *));
	va_end(ap);

	return s;
}

/* IPC */
int inbound_fd;
int outbound_fd;

struct child {
	pid_t		pid, ppid;
	int		fd;
	int		remote_fd;
	int		state;
};

enum {
	CS_NEW = 0,	/* created */
	CS_RUNNING,	/* initialized */
	CS_EXITING,	/* about to exit */
	CS_DONE,
};

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
		fds[i + 1].events = POLLIN;
	}
}

struct child *child_new(pid_t pid, pid_t ppid)
{
	struct child *child;
	int fdsv[2], i;

	if (nchildren == CHILDREN_MAX) {
		trace(0, "too many child processes\n");
		/* XXX */
		return;
	}

	child = xmalloc(sizeof(*child));
	child->pid = pid;
	child->ppid = ppid;

	i = socketpair(AF_UNIX, SOCK_STREAM, 0, fdsv);
	if (i) {
		fprintf(stderr, "Failed to create a pipe\n");
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

	new_array = xmalloc(nchildren - 1);
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
		fprintf(stderr, "child %d is in the wrong state %d\n",
			child->state);
	close(child->fd);
	close(child->remote_fd);
	free(child);
}

void children_wait(void)
{
	int i = 0, ret;

	while (i < nchildren) {
		if (waitpid(children[i]->pid, &ret, WNOHANG)) {
			struct child *child = children[i];

			trace(0, "child %d exited with %d code\n",
			      child->pid, ret);
			child_remove_by_idx(i);
			child_free(child);

			if (i < nchildren)
				continue;
		}

		i++;
	}
}

/* main loop */
int child_started(pid_t pid)
{
	int ret;
	struct lrc_message *m;

	for (;;) {
		ret = poll(fds, nchildren + 1, 1);
		if (ret > 0) {
			int i;
			for (i = 0; i < nchildren + 1 && ret; i++)
				if (fds[i].revents & POLLIN) {
					struct child *child;
					int vec[2];

					ret--;
					m = lrc_message_recv(fds[ret].fd);
					if (m->type == MT_HANDSHAKE) {
						child = child_find_by_pid(m->pid);
						if (child) {
							trace(0, "%d already exists\n", m->pid);
							free(m);
							continue;
						}

						fprintf(stderr, ">>> %d\n", m->pid);
						child = child_new(m->pid, 0);
						free(m);

						m = xmalloc(sizeof(*m) + sizeof(int) * 2);

						lrc_message_init(m);
						m->length = sizeof(int) * 2;
						vec[0] = child->fd;
						vec[1] = child->remote_fd;
						trace(0, "%d, %d\n", vec[0], vec[1]);
						memcpy(&m->payload.response.buf, vec, sizeof(int) * 2);

						m->payload.response.code = 1; /* XXX */
						m->type = MT_RESPONSE;
						lrc_message_send(fds[ret].fd, m);
						free(m);
					} else if (m->type == MT_FORK) {
						child = child_find_by_pid(m->payload.fork.child);
						if (child) {
							trace(0, "%d already exists\n",
							      m->payload.fork.child);
							free(m);
							continue;
						}
						fprintf(stderr, ">>> %d FORKED %d\n",
							m->pid, m->payload.fork.child);
						child = child_new(m->payload.fork.child, 0);
						free(m);
					}
				}
		}

		children_wait();

		if (!nchildren)
			break;
	}

	return 0;
}

int spawn(const char *cmd, char *const argv[])
{
	pid_t pid;
	int i = 0;
	int ret;

	/*if (verbosity >= VERB_DEBUG) {
		DBG("going to execute [%s]:", cmd);
		while (argv[i])
			output(ERR, VERB_DEBUG, " %s", argv[i++]);
		output(ERR, VERB_DEBUG, "\n");
		}*/

	pid = fork();
	if (pid) {
		return child_started(pid);
		//waitpid(-1, &ret, 0); /* <- fall through */
	} else {
		ret = execve(cmd, argv, environ);
		if (ret) {
			fprintf(stderr, "exec %s failed\n", cmd);
			exit(EXIT_FAILURE);
		}
	}

	/*if (verbosity >= VERB_DEBUG)
	{
		DBG("exit code: ");
		if (WIFEXITED(ret))
			output(ERR, VERB_DEBUG, "%d\n", WEXITSTATUS(ret));
		else if (WIFSIGNALED(ret))
			output(ERR, VERB_DEBUG, "SIG %d%s\n", WTERMSIG(ret),
			       WCOREDUMP(ret) ? " (core dumped)" : "");
			       }*/

	return ret;// (WIFEXITED(ret) && WEXITSTATUS(ret) == 0);
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
	if (i)
		fprintf(stderr, "Failed trying to execute %s: %m\n", executable);

	return i;
}
