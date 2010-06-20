/*
 * standard library wrappers
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

#ifndef RANDOMCRASH_LIBC_H
#define RANDOMCRASH_LIBC_H

extern bool lrc_is_up(void);

static inline size_t lrc_strlen(const char *s)
{
	size_t ret;

	if (lrc_is_up())
		ret = __lrc_orig_strlen(s);
	else
		for (ret = 0; *(s + ret); ret++);

	return ret;
}

static inline char *lrc_strcpy(char *dest, const char *src)
{
	size_t i;

	if (lrc_is_up())
		return __lrc_orig_strcpy(dest, src);

	for (i = 0; src[i]; i++)
		dest[i] = src[i];
	dest[i] = 0;
	return dest;
}

static inline int lrc_strcmp(const char *s1, const char *s2)
{
	int i;

	if (lrc_is_up())
		return __lrc_orig_strcmp(s1, s2);

	for (i = 0; s1[i] && s1[i] == s2[i]; i++);

	return s1[i] - s2[i];
}

static inline int lrc_strncmp(const char *s1, const char *s2, size_t n)
{
	int i;

	if (lrc_is_up())
		return __lrc_orig_strncmp(s1, s2, n);

	for (i = 0; i < n && s1[i] && s1[i] == s2[i]; i++);

	return s1[i] - s2[i];
}

static inline char *lrc_strchrnul(const char *s, int c)
{
	int i;

	if (lrc_is_up())
		return __lrc_orig_strchrnul(s, c);

	for (i = 0; s[i] && s[i] != c; i++);

	return (char *)s + i;
}

extern char **environ;

static inline char *lrc_getenv(const char *name)
{
	int i;
	size_t len = lrc_strlen(name);

	for (i = 0; environ[i]; i++) {
		if (!lrc_strncmp(name, environ[i], len) && environ[i][len] == '=')
			return environ[i] + len + 1;
	}

	return 0;
}

static inline ssize_t lrc_write(int fd, const void *buf, size_t count)
{
	if (lrc_is_up())
		return __lrc_orig_write(fd, buf, count);

	panic("lrc_write called before LRC is up.\n");

	return 0xdeadbeef;
}

static inline ssize_t lrc_read(int fd, void *buf, size_t count)
{
	if (lrc_is_up())
		return __lrc_orig_read(fd, buf, count);

	panic("lrc_read called before LRC is up.\n");

	return 0xdeadbeef;
}

#endif /* RANDOMCRASH_LIBC_H */

