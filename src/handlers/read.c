/*
 * read() handlers
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
#include <ctype.h>
#include <string.h>
#include "override.h"
#include "symbols.h"
#include "handlers.h"

static int read_randomcase_probe(struct override *o, void *ctxp)
{
	return 0;
}

static int read_randomcase_enter(struct override *o, void *ctxp)
{
	struct __lrc_callctx_read *args = ctxp;

	lrc_callctx_set_handler(args, &read_randomcase);

	return 0;
}

static int read_randomcase_exit(struct override *o, void *ctxp, void *retp)
{
	struct __lrc_callctx_read *args = ctxp;
	unsigned char *buf = args->buf;
	int i, ret = *(int *)retp;

	for (i = 0; i < ret; i++)
		buf[i] = (random() & 1)
			? toupper(buf[i])
			: tolower(buf[i]);
	return 0;
}

struct handler read_randomcase = {
	.fn_name	= "read",
	.handler_name	= "read_randomcase",
	.enabled	= 1,
	.probe_func	= read_randomcase_probe,
	.entry_func	= read_randomcase_enter,
	.exit_func	= read_randomcase_exit,
};
