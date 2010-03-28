#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#include "override.h"
#include "log.h"

/* a very simple allocation pool */
static void *mem_base;	/* base of the pool */
static void *mem_free;	/* first free address within the pool */

static int lrc_pages = 2; /* tunable? */
static size_t memsz;

void lrc_initmem(void)
{
	memsz = lrc_pages * sysconf(_SC_PAGE_SIZE);
	mem_base = mmap(NULL, memsz, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (mem_base == MAP_FAILED)
		panic("mmap failed");

	log_print(LL_PINFO, "reserved %d bytes of memory for LRC\n", memsz);
	mem_free = mem_base;
}

size_t lrc_get_free_mem(void)
{
	return memsz - ((unsigned long)mem_free - (unsigned long)mem_base);
}

void *lrc_alloc(size_t size)
{
	void *ptr;

	/* FIXME: this is absolutely thread-unsafe, but I'm just prototyping */
	if (lrc_get_free_mem() < size)
		return NULL;

	ptr = mem_free;
	mem_free += size;

	return ptr;
}

