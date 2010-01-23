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
extern struct handler *acct_handlers[];
extern struct handler *handlers[];

#define MAXQUEUE 32

int __lrc_call_entry(struct override *o, void *ctxp)
{
	struct handler *queue[MAXQUEUE];
	int i, qlast = 0;

	fprintf(stderr, "--- %s() entry ---\n", o->name);

	/* first, run all the enabled accounting handlers */
	for (i = 0; acct_handlers[i]; i++)
		if (!strcmp(acct_handlers[i]->fn_name, o->name) &&
		    acct_handlers[i]->enabled &&
		    acct_handlers[i]->probe_func &&
		    !acct_handlers[i]->probe_func(o, ctxp))
			;

	for (i = 0; handlers[i] && qlast < MAXQUEUE; i++)
		if (!strcmp(handlers[i]->fn_name, o->name) &&
		    handlers[i]->enabled &&
		    handlers[i]->probe_func &&
		    !handlers[i]->probe_func(o, ctxp))
			queue[qlast++] = handlers[i];

	if (!qlast) {
		fprintf(stderr, "warning: no handlers for %s call\n", o->name);
		return 0;
	}

	/* XXX: need to pick a random one */
	queue[0]->entry_func(o, ctxp);

	return 0;
}

void __lrc_call_exit(struct override *o, void *ctxp, void *retp)
{
	int i;
	struct handler *handler = ((struct __lrc_callctx *)ctxp)->priv;

	fprintf(stderr, "--- %s() exit, ret=%d ---\n", o->name, retp ? *(int *)retp : 0);

	if (handler && handler->exit_func)
		handler->exit_func(o, ctxp, retp);
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
