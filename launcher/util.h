#ifndef __LAUNCHER_UTIL_H__
#define __LAUNCHER_UTIL_H__

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <linux/limits.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <errno.h>

#ifdef __LRC_DEBUG__
#define DEVELOPER_MODE
#endif

#ifndef LRC_CONFIG_ENV
#define LRC_CONFIG_ENV "LRC_CONFIG"
#endif

#ifndef LRC_SRC_BASE_DIR
#define LRC_SRC_BASE_DIR get_current_dir_name()
#endif

/* stolen from omap-u-boot-utils, written by Nishanth Menon <nm@ti.com> */
#define __COLOR(m)	"\x1b[" # m
#define __RED		__COLOR(1;31m)
#define __GREEN		__COLOR(1;32m)
#define __YELLOW	__COLOR(1;33m)
#define __BLUE		__COLOR(1;34m)
#define __CYAN		__COLOR(1;36m)
#define __RESET		__COLOR(m)

extern unsigned int verbosity;

#define INFO		0x01
#define ERROR		0x02
#define DBG_BASIC	0x04
#define DBG_CHILD	0x08
#define DBG_PROTO	0x10
#define DBG_ANAL	0x20
#define DBG_MASK	0x3f

#define output(s, l, f, args...)			\
        do {						\
		if ((verbosity & (l)) == (l))		\
			fprintf(s, f, ## args);		\
	} while (0)

#define color_output(c, s, l, f, args ...)	\
	output(s, l, c f __RESET, ## args)

#ifdef DEVELOPER_MODE
#define trace(n, fmt, args ...) \
	color_output(__YELLOW, stderr, n, "<" #n ">: " fmt, ## args)
#else
#define trace(n, fmt, args ...)
#endif

#define log(f, args ...)			\
	color_output(__GREEN, stdout, DBG_CHILD, f, ## args)

#define info(f, args ...)				\
	color_output(__CYAN, stdout, INFO, f, ## args)

#define error(f, args ...)				\
	color_output(__RED, stderr, ERROR, f, ## args)

#ifdef DEVELOPER_MODE
void sigabrt_dumper(int sig);
#else
//void sigabrt_dumper(int sig) {}
#endif /* !DEVELOPER_MODE */

void *xmalloc(size_t len);
void *xrealloc(void *ptr, size_t len);
char *xstrdup(const char *str);

#define xfree(x)						\
	do {							\
		void **__x = (void **)&(x);			\
		if (*__x) {					\
			free(*__x);				\
			*__x = NULL;				\
		}						\
	} while (0);

int xpoll(struct pollfd *fds, nfds_t nfds, int timeout);
char *append_string(char *string, char *data);
char *append_strings(char *string, int n, ...);
ssize_t __vread(int is_pipe, char **out, const char *openstr);
ssize_t vread_file(char **out, const char *openstr);
ssize_t read_file(char **out, const char *fmt, ...);

#endif
