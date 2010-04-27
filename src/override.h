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

/*
 * a set of callbacks to be run for a certain intercepted callback
 */
struct handler {
	int	enabled:1;
	char	*fn_name;
	char	*handler_name;
	void	(*fini_func)(void);
	int	(*probe_func)(struct override *, void *);
	int	(*entry_func)(struct override *, void *);
	int	(*exit_func)(struct override *, void *, void *);
};

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
