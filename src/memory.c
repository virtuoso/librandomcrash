/*
 * librandomcrash internal memory management
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

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#include "override.h"
#include "log.h"
#include "smalloc.h"

/* a very simple allocation pool */
static void *mem_base;	/* base of the pool */
static void *mem_free;	/* first free address within the pool */

static int lrc_pages = 200; /* tunable? */
static size_t memsz;
static smalloc_t *s;

void lrc_initmem(void)
{
	memsz = lrc_pages * sysconf(_SC_PAGE_SIZE);
	mem_base = mmap(NULL, memsz, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (mem_base == MAP_FAILED)
		panic("mmap failed");

	debug("reserved %d bytes of memory for LRC\n", memsz);
	mem_free = mem_base;
	s = smalloc_init(mem_base, memsz, NULL);
}

/*size_t lrc_get_free_mem(void)
{
	return memsz - ((unsigned long)mem_free - (unsigned long)mem_base);
	}*/

void *lrc_alloc(size_t size)
{
	void *ptr;

	return smalloc(s, size);
	/* FIXME: this is absolutely thread-unsafe, but I'm just prototyping */
	if (lrc_get_free_mem() < size)
		return NULL;

	ptr = mem_free;
	mem_free += size;

	return ptr;
}

void lrc_free(void *ptr)
{
	sfree(s, ptr);
}
