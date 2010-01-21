#include "override.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <errno.h>

#include "symbols.h" /* XXX: move away */

static const char my_name[] = "librandomcrash";
static const char my_ver[] = "0.0.1";

void basic_nastiness(struct override *o)
{
	fprintf(stderr, "--------------------\n");
}

extern struct override *lrc_overrides[];

/*static const struct override overrides[] = {
	{
		.name = "open",
		.new_func = basic_nastiness,
		.nargs = 3,
	}, {
		.name = "close",
		.new_func = basic_nastiness,
		.nargs = 1,
	}
	};*/

int __lrc_call_entry(struct override *o, void *ctxp)
{
	fprintf(stderr, "--- %s() entry ---\n", o->name);
	return 0;
}

void __lrc_call_exit(struct override *o, void *ctxp, void *retp)
{
	fprintf(stderr, "--- %s() exit, ret=%d ---\n", o->name, retp ? *(int *)retp : 0);

	/* let's provide an example */
	if (!strcmp(o->name, "read")) {
		/* bwahaha */
		struct __lrc_callctx_read *args = ctxp;
		unsigned char *buf = args->buf;
		int i, ret = *(int *)retp;

		for (i = 0; i < ret; i++)
			buf[i] = (random() & 1)
				? toupper(buf[i])
				: tolower(buf[i]);
	}
}

void __ctor lrc_init(void)
{
	int i;

	fprintf(stderr, "%s, %s initializing\n", my_name, my_ver);
	for (i = 0; lrc_overrides[i]; i++) {
		fprintf(stderr, "=> loading %s wrapper\n",
			lrc_overrides[i]->name);
		lrc_overrides[i]->orig_func = dlsym(RTLD_NEXT,
						    lrc_overrides[i]->name);
	}
}

void __dtor lrc_done(void)
{
}

#ifdef __YEAH_RIGHT__
int main (void)
{
        return 0;
}
#endif
