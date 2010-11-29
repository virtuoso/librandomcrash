#include <execinfo.h>

void lrc_dump_trace(int fd)
{
	void *buffer[16];
	int n;

	n = backtrace(buffer, 16);
	backtrace_symbols_fd(buffer, n, fd);
}
