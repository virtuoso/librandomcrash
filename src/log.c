#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include "override.h"
#include "log.h"

struct log_output {
	FILE	*file;
	char	*path;
};

#define MAX_OUTPUT 4

static struct log_output *log_output[MAX_OUTPUT];
static int log_noutputs;

char log_dir[] = "/tmp";

void __noret panic(const char *msg)
{
	abort();
}

static int log_add_output(FILE *file, char *path)
{
	struct log_output *o;

	if (log_noutputs == MAX_OUTPUT)
		return -1;

	o = malloc(sizeof(*o));
	if (!o)
		return -1;

	o->path = strdup(path);
	if (!o->path) {
		free(o);
		return -1;
	}

	o->file = file;
	log_output[log_noutputs++] = o;

	return 0;
}

struct log_level {
	const char	*prefix;
	int		all_outputs;
};

static struct log_level log_level[] = {
	{ "BROADCAST",	1 },
	{ "---INFO",	0 }, /* lrc message */
	{ "   INFO",	0 }, /* process related */
	{ "---WARN",	1 },
	{ "   WARN",	0 },
	{ "---ERR",	1 },
	{ "   ERR",	1 },
};

void log_print(int level, const char *fmt, ...)
{
	va_list ap;

	if (level >= LL_LAST)
		return;

	fprintf(log_output[0]->file, "%s: ", log_level[level].prefix);
	va_start(ap, fmt);
	vfprintf(log_output[0]->file, fmt, ap);
	va_end(ap);
}

void log_init(void)
{
	char template[PATH_MAX];
	int fd, r;

	r = snprintf(template, PATH_MAX, "%s/librandomcrash_%d.XXXXXX",
		     log_dir, getpid());

	if (r < 0 || r >= PATH_MAX)
		goto fail;

	fd = mkstemp(template);
	if (fd == -1)
		goto fail;

	r = log_add_output(fdopen(fd, "w"), template);
	if (r == -1)
		goto fail;

#ifdef __LRC_DEBUG__
	r = log_add_output(stderr, "stderr");
	if (r == -1)
		goto fail;
#endif

	return;

fail:
	panic("Can't initialize logging");
}
