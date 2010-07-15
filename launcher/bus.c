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
#define panic(x) abort()

#include "../common/bus.c"
