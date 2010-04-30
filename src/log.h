/*
 * librandomcrash logging
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

#ifndef RANDOMCRASH_LOG_H
#define RANDOMCRASH_LOG_H

enum {
	LL_BCAST = 0,
	LL_OINFO,
	LL_PINFO,
	LL_OWARN,
	LL_PWARN,
	LL_OERR,
	LL_PERR,
	LL_LAST,
};

void log_init(void);
void log_print(int level, const char *fmt, ...);
void __noret panic(const char *msg);

#ifdef __LRC_DEBUG__
#define debug(fmt, args ...)			\
	log_print(LL_OINFO, "%s(): " fmt, __func__, ## args)
#define warn(fmt, args ...)			\
	log_print(LL_OWARN, "%s(): " fmt, __func__, ## args)
#else
#define debug(fmt, args ...) do {} while (0)
#define warn(fmt, args ...) do {} while (0)
#endif

#endif
