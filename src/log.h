#ifndef RANDOMCRASH_LOG_H
#define RANDOMCRASH_LOG_H

enum {
	LL_BCAST = 0,
	LL_OINFO,
	LL_PINFO,
	LL_OWARN,
	LL_PWARN,
	LL_OERR,
	LL_PERR,
	LL_LAST,
};

void log_init(void);
void log_print(int level, const char *fmt, ...);
void __noret panic(const char *msg);

#ifdef __LRC_DEBUG__
#define debug(fmt, args ...)			\
	log_print(LL_OINFO, "%s(): " fmt, __func__, ## args)
#define warn(fmt, args ...)			\
	log_print(LL_OWARN, "%s(): " fmt, __func__, ## args)
#else
#define debug(fmt, args ...) do {} while (0)
#define warn(fmt, args ...) do {} while (0)
#endif

#endif
