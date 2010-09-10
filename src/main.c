/*
 * librandomcrash core
 *
 * Copyright (C) 2010 Alexander Shishkin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>

#include "override.h"
#include "handlers.h"
#include "symbols.h"
#include "log.h"
#include "lrc-libc.h"
#include "memory.h"
#include "conf.h"

static const char my_name[] = PACKAGE;
static const char my_ver[] = VERSION;

static __thread int lrc_up;

/*
 * lrc_depth explicitly guards against original libc functions
 * being overridden when called from lrc code
 */
static __thread unsigned int lrc_depth;
static __thread sigset_t lrc_saved_sigset;

/*
 * Block all signals and save previous signal mask
 */
static void lrc_sigblock(void)
{
	sigset_t set;

	lrc_sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, &lrc_saved_sigset);

	return sigset;
}

/*
 * Restore signal mask
 */
static void lrc_sigunblock(void)
{
	sigprocmask(SIG_SETMASK, &lrc_saved_sigset, NULL);
	lrc_sigemptyset(&lrc_saved_sigset);
}

/*
 * Indicate that the execution is within lrc
 */
static void lrc_enter(void)
{
	lrc_sigblock();
	lrc_depth++;
	lrc_sigunblock();
}

static bool lrc_try_enter(void)
{
	lrc_sigblock();

	if (lrc_depth) {
		lrc_sigunblock();
		return false;
	}

	lrc_depth++;
	lrc_sigunblock();

	return true;
}

static void lrc_leave(void)
{
	if (!lrc_leave)
		panic("inconsistent lrc depth");

	lrc_depth--;
}

bool lrc_is_up(void)
{
	return !!lrc_up;
}

extern struct override *lrc_overrides[];
extern struct handler *acct_handlers[];
extern struct handler *handlers[];

#define MAXQUEUE 32

void __ctor lrc_init(void);

static unsigned long int lrc_callno = 0;

int __lrc_call_entry(struct override *o, void *ctxp)
{
	struct handler *queue[MAXQUEUE];
	int i, qlast = 0, ret = 0;

	if (!lrc_try_enter())
		return 0;

	if (!lrc_up)
		panic("lrc is not up where it ought to be.\n");

	lrc_callno++;

	debug("%s() entry, call nr: %lu\n", o->name, lrc_callno);

	/* first, run the first enabled accounting handler for this call */
	for (i = 0; acct_handlers[i]; i++)
		if (!lrc_strcmp(acct_handlers[i]->fn_name, o->name) &&
		    acct_handlers[i]->enabled &&
		    acct_handlers[i]->probe_func &&
		    !acct_handlers[i]->probe_func(o, ctxp))
			break;

	if (!lrc_conf_long(CONF_NOCRASH)) {
		if (lrc_callno <= lrc_conf_long(CONF_SKIPCALLS)) {
			debug("%s() skipping\n", o->name);
			goto out;
		}

		for (i = 0; handlers[i] && qlast < MAXQUEUE; i++)
			if (!lrc_strcmp(handlers[i]->fn_name, o->name) &&
			    handlers[i]->enabled &&
			    handlers[i]->probe_func &&
			    !handlers[i]->probe_func(o, ctxp))
				queue[qlast++] = handlers[i];

		if (!qlast) {
			warn("no handlers for %s call\n", o->name);
			goto out;
		}

		/* XXX: need to pick a random one */
		ret = queue[0]->entry_func(o, ctxp);
	}

out:
	lrc_leave();

	return ret;
}

void __lrc_call_exit(struct override *o, void *ctxp, void *retp)
{
	struct lrcpriv_callctx *callctx =
		&((struct __lrc_callctx *)ctxp)->callctx;

	if (!lrc_try_enter())
		return;

	debug("%s() exit, ret=%d\n", o->name,
		  retp ? *(int *)retp : 0);

	if (callctx->acct_handler && callctx->acct_handler->exit_func)
		callctx->acct_handler->exit_func(o, ctxp, retp);

	if (callctx->handler && callctx->handler->exit_func)
		callctx->handler->exit_func(o, ctxp, retp);
	lrc_leave();
}

EXPORT void __ctor lrc_init(void)
{
	int i;

	if (lrc_up)
		return;

	/* we can't actually do anything before we do this */
	for (i = 0; lrc_overrides[i]; i++)
		lrc_overrides[i]->orig_func = dlsym(RTLD_NEXT,
						    lrc_overrides[i]->name);

	lrc_sigemptyset(&lrc_saved_sigset);

	lrc_enter();

	log_init();
	log_print(LL_OINFO, "%s, %s initialized\n", my_name, my_ver);

	lrc_initmem();
	lrc_configure();

	lrc_up++;
	lrc_leave();
}

EXPORT void __dtor lrc_done(void)
{
	int i;

	for (i = 0; acct_handlers[i]; i++)
		if (acct_handlers[i]->fini_func)
			acct_handlers[i]->fini_func();
}

#ifdef __YEAH_RIGHT__
int main (void)
{
        return 0;
}
#endif
