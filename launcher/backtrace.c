#include "util.h"

void lrc_dump_trace(int fd);

#include "../src/backtrace.c"

void sigabrt_dumper(int sig)
{
	info("signal %d received\n", sig);
	lrc_dump_trace(2);
}
