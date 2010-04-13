#include <stdlib.h>
#include "override.h"
#include "symbols.h"
#include "handlers.h"

static int readdir_retnull_probe(struct override *o, void *ctxp)
{
	return 0;
}

static int readdir_retnull_enter(struct override *o, void *ctxp)
{
	struct __lrc_callctx_readdir *args = ctxp;

	lrc_callctx_set_handler(args, &readdir_retnull);

	return 0;
}

static int readdir_retnull_exit(struct override *o, void *ctxp, void *retp)
{
        struct dirent **de = retp;
        if (*de)
                *de = NULL;

	return 0;
}

struct handler readdir_retnull = {
	.fn_name	= "readdir",
	.handler_name	= "readdir_retnull",
	.enabled	= 1,
	.probe_func	= readdir_retnull_probe,
	.entry_func	= readdir_retnull_enter,
	.exit_func	= readdir_retnull_exit,
};
