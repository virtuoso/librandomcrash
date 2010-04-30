/*
 * overrides' data types
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

#ifndef RANDOMCRASH_OVERRIDE_H
#define RANDOMCRASH_OVERRIDE_H

typedef void (*fnptr_t)(void);

struct override {
	const char *name;
	fnptr_t orig_func;
	fnptr_t new_func;
	int nargs;
	unsigned int flags;
};

struct handler;

/*
 * lrc-private call context
 */
struct lrcpriv_callctx {
	struct handler	*acct_handler;
	struct handler	*handler;
};

/*
 * common part of all callctx structures
 */
struct __lrc_callctx {
	struct lrcpriv_callctx callctx;
};

#define lrc_callctx_set_handler(__c, __h)				\
	do {								\
		(__c)->callctx.handler = (__h);				\
	} while (0);

#define lrc_callctx_set_acct_handler(__c, __h)				\
	do {								\
		(__c)->callctx.acct_handler = (__h);			\
	} while (0);

#define arrsz(__a) (sizeof(__a) / sizeof(__a[0]))

#define __ctor __attribute__((constructor))
#define __dtor __attribute__((destructor))
#define __noret __attribute__((noreturn))

int __lrc_call_entry(struct override *o, void *ctxp);
void __lrc_call_exit(struct override *o, void *ctxp, void *retp);

void lrc_going_to_crash(const char *site, const char *cond);

#define CRASH_COND(cond)				\
	if ((cond)) {					\
		lrc_going_to_crash(__func__, # cond);	\
	}

/* XXX: this should check /proc/self/maps instead */
#define INVALID_PTR(ptr) (ptr == NULL)

#endif
