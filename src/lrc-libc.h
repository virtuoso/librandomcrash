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

	if (__lrc_orig_strlen_is_callable())
		ret = __lrc_orig_strlen(s);
	else
		for (ret = 0; *(s + ret); ret++);

	return ret;
}

static inline char *lrc_strcpy(char *dest, const char *src)
{
	size_t i;

	if (__lrc_orig_strcpy_is_callable())
		return __lrc_orig_strcpy(dest, src);

	for (i = 0; src[i]; i++)
		dest[i] = src[i];
	dest[i] = 0;
	return dest;
}

static inline int lrc_strcmp(const char *s1, const char *s2)
{
	int i;

	if (__lrc_orig_strcmp_is_callable())
		return __lrc_orig_strcmp(s1, s2);

	for (i = 0; s1[i] && s1[i] == s2[i]; i++);

	return s1[i] - s2[i];
}

static inline int lrc_strncmp(const char *s1, const char *s2, size_t n)
{
	int i;

	if (__lrc_orig_strncmp_is_callable())
		return __lrc_orig_strncmp(s1, s2, n);

	for (i = 0; i < n && s1[i] && s1[i] == s2[i]; i++);

	return i == n ? 0 : s1[i] - s2[i];
}

static inline char *lrc_strchrnul(const char *s, int c)
{
	int i;

	if (__lrc_orig_strchrnul_is_callable())
		return __lrc_orig_strchrnul(s, c);

	for (i = 0; s[i] && s[i] != c; i++);

	return (char *)s + i;
}

static inline pid_t lrc_gettid(void)
{
	/* XXX: gettid with glibc would require syscall() and I'm lazy now */
	if (__lrc_orig_getpid_is_callable())
		return __lrc_orig_getpid();

	return -1;
}

static inline void *lrc_memset(void *s, int c, size_t n)
{
	int i;

	if (__lrc_orig_memset_is_callable())
		return __lrc_orig_memset(s, c, n);

	for (i = 0; i < n; i++)
		((unsigned char *)s)[i] = c;

	return s;
}

static inline void *lrc_memcpy(void *dest, const void *src, size_t n)
{
	size_t i;

	if (__lrc_orig_memcpy_is_callable())
		return __lrc_orig_memcpy(dest, src, n);

	for (i = 0; i < n; i++)
		*(unsigned char *)dest++ = *(unsigned char *)src++;

	return dest;
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
	if (__lrc_orig_write_is_callable())
		return __lrc_orig_write(fd, buf, count);

	panic("lrc_write called, but no libc function available.\n");

	return 0xdeadbeef;
}

static inline ssize_t lrc_read(int fd, void *buf, size_t count)
{
	if (__lrc_orig_read_is_callable())
		return __lrc_orig_read(fd, buf, count);

	panic("lrc_read called, but no libc function available.\n");

	return 0xdeadbeef;
}

/*
 * Signal-related stuff follows
 */

static inline int lrc_sigemptyset(sigset_t *set)
{
	if (__lrc_orig_sigemptyset_is_callable())
		return __lrc_orig_sigemptyset(set);

	panic("lrc_sigemptyset called, but no libc function available.\n");

	return 0xdeadbeef;
}

static inline int lrc_sigfillset(sigset_t *set)
{
	if (__lrc_orig_sigfillset_is_callable())
		return __lrc_orig_sigfillset(set);

	panic("lrc_sigfillset called, but no libc function available.\n");

	return 0xdeadbeef;
}

#endif /* RANDOMCRASH_LIBC_H */

