/*
 * procps maps scanner
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

#ifndef __LAUNCHER_MAPS_H__
#define __LAUNCHER_MAPS_H__

#define VA_OPT_READ	(1 << 0)
#define VA_OPT_WRITE	(1 << 1)
#define VA_OPT_EXEC	(1 << 2)
#define VA_OPT_OURS	(1 << 3)

void maps_print(pid_t pid);

bool validate_addr(pid_t pid, unsigned long addr, unsigned int opts);

void maps_save(pid_t pid, unsigned int opts);

#endif /* !__LAUNCHER_MAPS_H__ */
