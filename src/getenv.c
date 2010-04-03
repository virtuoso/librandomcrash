#include <stdlib.h>
#include "override.h"
#include "symbols.h"
#include "handlers.h"
#include "log.h"

static int getenv_retnull_probe(struct override *o, void *ctxp)
{
	return 0;
}

static int getenv_retnull_enter(struct override *o, void *ctxp)
{
	struct __lrc_callctx_getenv *args = ctxp;

	lrc_callctx_set_handler(args, &getenv_retnull);

	return 0;
}

static int getenv_retnull_exit(struct override *o, void *ctxp, void *retp)
{
        struct __lrc_callctx_getenv *args = ctxp;
        unsigned char *name = args->name;
	char **pval = retp;

	*pval = NULL;

	return 0;
}

struct handler getenv_retnull = {
	.fn_name	= "getenv",
	.handler_name	= "getenv_retnull",
	.enabled	= 1,
	.probe_func	= getenv_retnull_probe,
	.entry_func	= getenv_retnull_enter,
	.exit_func	= getenv_retnull_exit,
};
