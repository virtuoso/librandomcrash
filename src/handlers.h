/*
 * handlers' data types
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

#ifndef RANDOMCRASH_HANDLERS_H
#define RANDOMCRASH_HANDLERS_H

/*
 * a set of callbacks to be run for a certain intercepted call
 * see doc/handlers.txt for a brief introduction
 */
struct handler {
	int	enabled:1;
	/* name of an intercepted call this handler deals with */
	char	*fn_name;
	/* name of this handler, must be unique */
	char	*handler_name;
	/* this will be called upon program exit for all accounting handlers */
	void	(*fini_func)(void);
	/* decide if the handler wants to run */
	int	(*probe_func)(struct override *, void *);
	/* called upon library function entry */
	int	(*entry_func)(struct override *, void *);
	/* called upon library function exit */
	int	(*exit_func)(struct override *, void *, void *);
};

/* all the handlers that we know about */
extern struct handler read_randomcase;
extern struct handler open_stat;
extern struct handler opendir_retnull;
extern struct handler getenv_retnull;
extern struct handler readdir_retnull;

#endif
