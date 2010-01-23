#include <stdlib.h>
#include "override.h"
#include "symbols.h"
#include "handlers.h"

static int read_randomcase_probe(struct override *o, void *ctxp)
{
	return 0;
}

static int read_randomcase_enter(struct override *o, void *ctxp)
{
	struct __lrc_callctx_read *args = ctxp;

	args->priv = &read_randomcase;

	return 0;
}

static int read_randomcase_exit(struct override *o, void *ctxp, void *retp)
{
	struct __lrc_callctx_read *args = ctxp;
	unsigned char *buf = args->buf;
	int i, ret = *(int *)retp;

	for (i = 0; i < ret; i++)
		buf[i] = (random() & 1)
			? toupper(buf[i])
			: tolower(buf[i]);
	return 0;
}

struct handler read_randomcase = {
	.fn_name	= "read",
	.handler_name	= "read_randomcase",
	.enabled	= 1,
	.probe_func	= read_randomcase_probe,
	.entry_func	= read_randomcase_enter,
	.exit_func	= read_randomcase_exit,
};
