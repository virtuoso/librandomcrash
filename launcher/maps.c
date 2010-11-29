/*
 * procfs maps scanner
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

#include "util.h"
#include "maps.h"

struct maprec {
	unsigned long	start;
	unsigned long	end;
	bool		read;
	bool		write;
	bool		execute;
	bool		shared;
	off_t		offset;
	dev_t		device;
	ino_t		inode;
	char		*path;
};

#if ULONG_MAX == 0xffffffff
#define OFFFMT "ll"
#else
#define OFFFMT "l"
#endif

/*
 * traverse through a given pid's maps, call a callback for
 * each record
 */
static void maps_scan(pid_t pid, int (*fn)(struct maprec *, void *),
		      void *data)
{
	struct maprec *current;
	char *maps;
	ssize_t l;
	int pos = 0;

	l = read_file(&maps, "/proc/%d/maps", pid);
	if (l <= 0) {
		/* XXX */
		return;
	}

	while (pos < l) {
		unsigned int maj, min, ppos, n;
		char prot[5];
		int ret;

		current = xmalloc(sizeof(*current));
		n = sscanf(maps + pos,
			   "%lx-%lx %4s %" OFFFMT "x %02x:%02x %lu%n",
			   &current->start, &current->end, prot,
			   &current->offset, &maj, &min, &current->inode,
			   &ppos);

		current->read = prot[0] == 'r';
		current->write = prot[1] == 'w';
		current->execute = prot[2] == 'x';
		current->shared = prot[3] == 's';
		current->device = maj << 8 | min;

		for (pos += ppos; maps[pos] && maps[pos] == ' '; pos++);

		if (maps[pos] != '\n') {
			int end = pos;

			for (; maps[end] && maps[end] != '\n'; end++);
			maps[end] = 0;
			current->path = xstrdup(&maps[pos]);
			pos = end;
		} else
			current->path = NULL;

		pos++;

		ret = fn(current, data);

		free(current->path);
		free(current);

		if (ret)
			break;
	}

	free(maps);
}

static int dump_callback(struct maprec *current, void *data)
{
	/* XXX: formatting? */
	fprintf(stderr, "%lx...%lx %s\n", current->start, current->end,
		current->path);

	return 0;
}

void maps_print(pid_t pid)
{
	maps_scan(pid, dump_callback, NULL);
}

struct addr_callback_data {
	unsigned long	addr;
	unsigned int	opts;
	bool		hit;
};

static int addr_callback(struct maprec *current, void *data)
{
	struct addr_callback_data *a = data;

	if ((a->opts & VA_OPT_READ) && !current->read)
		return 0;

	if ((a->opts & VA_OPT_WRITE) && !current->write)
		return 0;

	if ((a->opts & VA_OPT_EXEC) && !current->execute)
		return 0;

	if (current->start <= a->addr && current->end > a->addr) {
		a->hit = true;
		return 1;
	}

	return 0;
}

/*
 * check if a given address is valid in pid's address space;
 * opts specify filtering for RWX type access
 */
bool validate_addr(pid_t pid, unsigned long addr, unsigned int opts)
{
	struct addr_callback_data a = {
		.addr = addr,
		.opts = opts,
		.hit  = false,
	};

	maps_scan(pid, addr_callback, &a);

	return a.hit;
}

struct save_callback_data {
	FILE *out;
	unsigned int opts;
};

static int save_callback(struct maprec *current, void *data)
{
	struct save_callback_data *s = data;

	if ((s->opts & VA_OPT_OURS) &&
	    (!current->path || !strstr(current->path, "librandomcrash.so")))
		return 0;

	fprintf(s->out, "%lx-%lx %c%c%c%c %lx %02x:%02x %lu",
		current->start, current->end,
		current->read ? 'r' : '-',
		current->write ? 'w' : '-',
		current->execute ? 'x' : '-',
		current->shared ? 's' : 'p',
		current->offset,
		(unsigned int)(current->device >> 8),
		(unsigned int)(current->device & 0xf), current->inode
		);
	if (current->path)
		fprintf(s->out, " %s", current->path);
	fputc('\n', s->out);

	return 0;
}

void maps_save(pid_t pid, unsigned int opts)
{
	struct save_callback_data s = {
		.opts = opts,
	};
	char filename[PATH_MAX];

	snprintf(filename, PATH_MAX, "/tmp/lrc_maps.%d", pid);
	s.out = fopen(filename, "w");
	if (!s.out)
		return;

	maps_scan(pid, save_callback, &s);
	fclose(s.out);
}
