#include <stdlib.h>
#include "override.h"
#include "symbols.h"
#include "handlers.h"

static int opendir_retnull_probe(struct override *o, void *ctxp)
{
	return 0;
}

static int opendir_retnull_enter(struct override *o, void *ctxp)
{
	struct __lrc_callctx_opendir *args = ctxp;

	lrc_callctx_set_handler(args, &opendir_retnull);

	return 0;
}

static int opendir_retnull_exit(struct override *o, void *ctxp, void *retp)
{
        DIR **dir = retp;
        if (*dir) {
                closedir(*dir);
                *dir = NULL;
        }

	return 0;
}

struct handler opendir_retnull = {
	.fn_name	= "opendir",
	.handler_name	= "opendir_retnull",
	.enabled	= 1,
	.probe_func	= opendir_retnull_probe,
	.entry_func	= opendir_retnull_enter,
	.exit_func	= opendir_retnull_exit,
};
