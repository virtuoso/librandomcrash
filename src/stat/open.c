#include <stdlib.h>
#include <stdio.h>
#include "override.h"
#include "symbols.h"
#include "handlers.h"
#include "log.h"

static int open_stat_probe(struct override *o, void *ctxp)
{
	struct __lrc_callctx_open *args = ctxp;

	args->priv = &open_stat;

	return 0;
}

static int open_stat_exit(struct override *o, void *ctxp, void *retp)
{
	struct __lrc_callctx_open *args = ctxp;
	const char *file = args->file;
	int i, ret = *(int *)retp;

	log_print(LL_PINFO, "opening \"%s\"\n", file);

	return 0;
}

struct handler open_stat = {
	.fn_name	= "open",
	.handler_name	= "open_stat",
	.enabled	= 1,
	.probe_func	= open_stat_probe,
	.exit_func	= open_stat_exit,
};
