#include "util.h"

void *xmalloc(size_t len)
{
	void *x = malloc(len);

	if (!x) {
		fprintf(stderr, "We're short on memory\n");
		exit(EXIT_FAILURE);
	}

	return x;
}

void *xrealloc(void *ptr, size_t len)
{
	void *x = realloc(ptr, len);

	if (!x) {
		fprintf(stderr, "We're short on memory\n");
		exit(EXIT_FAILURE);
	}

	return x;
}

char *xstrdup(const char *str)
{
	void *x = strdup(str);

	if (!x) {
		fprintf(stderr, "We're short on memory\n");
		exit(EXIT_FAILURE);
	}

	return x;
}

int xpoll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	int ret;

	assert(nfds);
	assert(fds);

	do {
		ret = poll(fds, nfds, timeout);
	} while (ret == -1 && errno == EINTR);

	if (ret == -1) {
		fprintf(stderr, "poll fails: %m\n");
		exit(EXIT_FAILURE);
	}

	return ret;
}

char *append_string(char *string, char *data)
{
	char *s = string;
	int sz, len;

	sz = string ? strlen(string) : 0;
	len = strlen(data);
	s = realloc(s, sz + len + 1);
	if (!s)
		exit(EXIT_FAILURE);
	memcpy(s + sz, data, len);
	s[sz + len] = 0;

	return s;
}

char *append_strings(char *string, int n, ...)
{
	va_list ap;
	int i;
	char *s = string;

	va_start(ap, n);
	for (i = 0; i < n; i++)
		s = append_string(s, va_arg(ap, char *));
	va_end(ap);

	return s;
}

ssize_t __vread(int is_pipe, char **out, const char *openstr)
{
        FILE *p;
        char *buf = NULL;
        size_t len = 0;
        int r = 1;

        p = is_pipe ? popen(openstr, "r") : fopen(openstr, "r");
        if (!p)
                return -1;

        while (!feof(p) && r) {
                buf = xrealloc(buf, len + BUFSIZ);
                if (!buf)
                        return -1;

                len +=
                r = read(fileno(p), buf + len, BUFSIZ);
        }

        if (is_pipe)
                pclose(p);
        else
                fclose(p);

        buf[len] = '\0';
        *out = buf;

        return len;
}

ssize_t vread_file(char **out, const char *openstr)
{
        return __vread(0, out, openstr);
}

ssize_t read_file(char **out, const char *fmt, ...)
{
        va_list args;
        char *opstr;
        int s;

        va_start(args, fmt);
        s = vasprintf(&opstr, fmt, args);
        va_end(args);

        if (s == -1)
                return -1;

        return vread_file(out, opstr);
}

