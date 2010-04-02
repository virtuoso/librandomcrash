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

#endif /* RANDOMCRASH_LIBC_H */

