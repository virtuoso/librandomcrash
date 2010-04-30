/*
 * opendir() handlers
 *
 * Copyright (C) 2010 Vladimir Levijev
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
#include "override.h"
#include "symbols.h"
#include "handlers.h"

static int opendir_retnull_probe(struct override *o, void *ctxp)
{
	return 0;
}

static int opendir_retnull_enter(struct override *o, void *ctxp)
{
	struct __lrc_callctx_opendir *args = ctxp;

	lrc_callctx_set_handler(args, &opendir_retnull);

	return 0;
}

static int opendir_retnull_exit(struct override *o, void *ctxp, void *retp)
{
        DIR **dir = retp;
        if (*dir) {
                closedir(*dir);
                *dir = NULL;
        }

	return 0;
}

struct handler opendir_retnull = {
	.fn_name	= "opendir",
	.handler_name	= "opendir_retnull",
	.enabled	= 1,
	.probe_func	= opendir_retnull_probe,
	.entry_func	= opendir_retnull_enter,
	.exit_func	= opendir_retnull_exit,
};
