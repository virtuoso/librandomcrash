#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include "override.h"
#include "log.h"

struct log_output {
	FILE	*file;
	char	path[PATH_MAX];
};

#define MAX_OUTPUT 4

static struct log_output log_output[MAX_OUTPUT];
static int log_noutputs;

char log_dir[] = "/tmp";

void __noret panic(const char *msg)
{
	abort();
}

static int log_add_output(FILE *file, char *path)
{
	struct log_output *o;

	if (!file)
		return -1;

	if (log_noutputs == MAX_OUTPUT)
		return -1;

	o = &log_output[log_noutputs];
	strncpy(o->path, path, PATH_MAX);

	log_output[log_noutputs].file = file;
	log_noutputs++;

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

	if (!log_noutputs)
		return;

	if (level >= LL_LAST)
		return;

	fprintf(log_output[0].file, "%s: ", log_level[level].prefix);
	va_start(ap, fmt);
	vfprintf(log_output[0].file, fmt, ap);
	va_end(ap);

	fflush(log_output[0].file);
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
