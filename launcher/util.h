#ifndef __LAUNCHER_UTIL_H__
#define __LAUNCHER_UTIL_H__

#ifdef __LRC_DEBUG__
#define DEVELOPER_MODE
#endif

#ifndef LRC_CONFIG_ENV
#define LRC_CONFIG_ENV "LRC_CONFIG"
#endif

#ifndef LRC_SRC_BASE_DIR
#define LRC_SRC_BASE_DIR get_current_dir_name()
#endif

#ifdef DEVELOPER_MODE
#define trace(n, fmt, args ...) fprintf(stderr, "> " #n " <: " fmt, ## args)
#else
#define trace(n, fmt, args ...)
#endif

void *xmalloc(size_t len);
void *xrealloc(void *ptr, size_t len);
char *xstrdup(const char *str);
int xpoll(struct pollfd *fds, nfds_t nfds, int timeout);
char *append_string(char *string, char *data);
char *append_strings(char *string, int n, ...);
ssize_t __vread(int is_pipe, char **out, const char *openstr);
ssize_t vread_file(char **out, const char *openstr);
ssize_t read_file(char **out, const char *fmt, ...);

#endif
