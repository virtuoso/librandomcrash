/*
 * librandomcrash configuration
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

#ifndef RANDOMCRASH_CONFIG_H
#define RANDOMCRASH_CONFIG_H

#ifndef LRC_CONFIG_ENV
#define LRC_CONFIG_ENV "LRC_CONFIG"
#endif

void lrc_configure(void);
char *lrc_conf_str(int what);
int lrc_conf_long(int what);

enum {
	CONF_LOGDIR = 0,
	CONF_NOCRASH,
	CONF_SKIPCALLS,
	CONF_FDIN,
	CONF_FDOUT,
};

#endif /* RANDOMCRASH_CONFIG_H */

