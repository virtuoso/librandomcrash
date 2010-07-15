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

#include "proto.h"

static const char my_name[] = "randomcrash";
static const char my_ver[] = VERSION;

static char lrc_path[PATH_MAX];

#ifdef __LRC_DEBUG__
#define DEVELOPER_MODE
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
int inbound_fd[2];
int outbound_fd[2];

struct child {
	pid_t		pid, ppid;
	int		inbound_fd[2];
	int		outbound_fd[2];
	struct child	*prev, *next;
};

static struct child *children;
static int nchildren;
static struct pollfd *fds;

static void children2fds(void)
{
	struct child *child = children;
	int i = 0;

	fds = realloc(fds, sizeof(*fds) * nchildren);
	fds[0].fd = inbound_fd[0];
	fds[0].events = POLLIN;

	for (i = 1; i < nchildren + 1; i++) {
		fds[i].fd = child->inbound_fd[0];
		fds[i].events = POLLIN;
		child = child->next;
	}
}

static inline void child_add_tail(struct child *child)
{
	if (children) {
		child->next = children;
		child->prev = children->prev;
		children->prev = child;
	} else
		children = child;
}

struct child *child_new(pid_t pid, pid_t ppid)
{
	struct child *child;

	child = xmalloc(sizeof(*child));
	child->pid = pid;
	child->ppid = ppid;
	child->next = child->prev = child;
	child_add_tail(child);

	pipe(child->inbound_fd);
	pipe(child->outbound_fd);

	nchildren++;

	children2fds();

	return child;
}

struct child *child_find_by_pid(pid_t pid)
{
	struct child *child = children;
	int i = 0;

	for (i = 1; i < nchildren + 1; i++) {
		if (child->pid == pid)
			return child;
		child = child->next;
	}

	return NULL;
}

void child_free(pid_t pid)
{
	struct child *child;

	child = child_find_by_pid(pid);
	if (!child) /* XXX: report */
		return;

	close(child->inbound_fd[0]);
	close(child->outbound_fd[1]);

	child->next->prev = child->prev;
	child->prev->next = child->next;
}

/* main loop */
int child_started(pid_t pid)
{
	int ret;
	struct lrc_message *m;

	for (;;) {
		ret = poll(fds, nchildren + 1, 0);
		if (ret > 0) {
			int i;
			for (i = 0; i < nchildren + 1 && ret; i++)
				if (fds[i].revents & POLLIN) {
					struct child *child;

					ret--;
					m = lrc_message_recv(fds[ret].fd);
					fprintf(stderr, ">>> %d\n", m->pid);
					child = child_new(m->pid, 0);
					free(m);

					m = xmalloc(sizeof(*m) + sizeof(int) * 2);

					lrc_message_init(m);
					m->length = sizeof(int) * 2;
					memcpy(&m->payload.response.buf[0], &child->inbound_fd[1], sizeof(int));
					memcpy(&m->payload.response.buf[4], &child->outbound_fd[0], sizeof(int));
					m->payload.response.code = 1; /* XXX */
					m->type = MT_RESPONSE;
					lrc_message_send(outbound_fd[1], m);
					free(m);
				}
		}

		/* XXX */
		if (waitpid(-1, &ret, WNOHANG))
			return ret;
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
		close(inbound_fd[1]);
		close(outbound_fd[0]);
		return child_started(pid);
		//waitpid(-1, &ret, 0); /* <- fall through */
	} else {
		close(inbound_fd[0]);
		close(outbound_fd[1]);
		/*close(1);
		  close(2);

		  dup2(fileno(OUT[LOG]), 1);
		  dup2(fileno(OUT[LOG]), 2);*/

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
	char *ipc_fdin = NULL;
	int no_crash = 0;
	char *logdir = NULL;
	unsigned long int skip_calls = 0;
	char buf[BUFSIZ];

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
	new_argv = xmalloc(argc - optind + 1);
	new_argv[0] = executable;

	for (i = 1; optind < argc; i++, optind++)
		new_argv[i] = xstrdup(argv[optind]);
	new_argv[i] = NULL;

	setenv("LD_PRELOAD", lrc_path, 1);

	/* IPC init */
	i = pipe(inbound_fd);
	if (i) {
		fprintf(stderr, "Failed to create a pipe\n");
		exit(EXIT_FAILURE);
	}

	asprintf(&ipc_fdin, "fdout=%d:", inbound_fd[1]);
	lrc_opts = append_string(lrc_opts, ipc_fdin);
	free(ipc_fdin);

	i = pipe(outbound_fd);
	if (i) {
		fprintf(stderr, "Failed to create a pipe\n");
		exit(EXIT_FAILURE);
	}

	asprintf(&ipc_fdin, "fdin=%d:", outbound_fd[0]);
	lrc_opts = append_string(lrc_opts, ipc_fdin);
	free(ipc_fdin);

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
