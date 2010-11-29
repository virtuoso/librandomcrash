#define _GNU_SOURCE
#include <sys/types.h>
#include <string.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <unistd.h>

#define lrc_memset memset
#define lrc_gettid() syscall(SYS_gettid)
#define lrc_write write
#define lrc_read read
#define lrc_alloc malloc
#define lrc_memcpy memcpy
#ifdef __LRC_DEBUG__
#include <signal.h>
void sigabrt_dumper(int);
#define panic(x) do { sigabrt_dumper(SIGABRT); abort(); } while (0)
#else
#define panic(x) abort()
#endif

#include "../common/bus.c"
