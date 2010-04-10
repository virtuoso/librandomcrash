#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>

static const char my_name[] = "randomcrash";
static const char my_ver[] = VERSION;

static char lrc_path[PATH_MAX];

#ifdef __LRC_DEBUG__
#define DEVELOPER_MODE
#endif

#ifndef LRC_CONFIG_ENV
#define LRC_CONFIG_ENV "LRC_CONFIG"
#endif

static const struct option options[] = {
	{ "help",	0, 0, 'h' },
	{ "no-crash",	0, 0, 'n' },
	{ "logdir",	1, 0, 'L' },
#ifdef DEVELOPER_MODE
	{ "local",	0, 0, 'l' },
#endif
	{ NULL,		0, 0, 0   },
};

static const char *optstr = "hnL:"
#ifdef DEVELOPER_MODE
	"l"
#endif
	;

static const char *optdesc[] = {
	"display this help text",
	"don't apply nastiness, just watch",
	"specify directory to be used for logging",
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

int main(int argc, char *const argv[])
{
	int loptidx, c, i;
	char **new_argv;
	char *executable;
	char *lrc_opts = NULL;
	int no_crash = 0;
	char *logdir = NULL;

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

#ifdef DEVELOPER_MODE
			case 'l':
				snprintf(lrc_path, PATH_MAX,
					 "%s/src/.libs/librandomcrash.so.%s",
					 get_current_dir_name(), my_ver);
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

	if (no_crash)
		lrc_opts = append_string(lrc_opts, "no-crash:");

	if (logdir)
		lrc_opts = append_strings(lrc_opts, 3, "logdir=", logdir, ":");

	setenv(LRC_CONFIG_ENV, lrc_opts ? lrc_opts : "", 1);

	/* perhaps it makes sense to fork and handle the exit codes */
	i = execve(executable, new_argv, environ);
	fprintf(stderr, "Failed trying to execute %s: %m\n", executable);

	return i;
}
