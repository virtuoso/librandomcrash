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

/* main loop */
int child_started(pid_t pid)
{
	struct pollfd fds = {
		.fd = inbound_fd[0],
		.events = POLLIN,
	};
	char buf[512];
	int ret;

	for (;;) {
		poll(&fds, 1, 1000);
		if (fds.revents & POLLIN) {
			read(fds.fd, buf, 512);
			fprintf(stderr, ">>> %s\n", buf);
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
		return child_started(pid);
		//waitpid(-1, &ret, 0); /* <- fall through */
	} else {
		close(inbound_fd[0]);
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

	asprintf(&ipc_fdin, "fd=%d:", inbound_fd[1]);
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

	/* perhaps it makes sense to fork and handle the exit codes */
	i = spawn(executable, new_argv);
	if (i)
		fprintf(stderr, "Failed trying to execute %s: %m\n", executable);

	return i;
}
