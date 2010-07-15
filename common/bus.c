#include <sys/types.h>
#include "../common/proto.h"

int lrc_message_init(struct lrc_message *m)
{
	lrc_memset(m, 0, sizeof(*m));
	m->pid = lrc_gettid();

	return 0;
}

int lrc_message_send(int fd, struct lrc_message *m)
{
	return lrc_write(fd, m, lrc_message_size(m));
}

struct lrc_message *lrc_message_recv(int fd)
{
	ssize_t l;
	struct lrc_message *m, tmp;

	l = lrc_read(fd, &tmp, sizeof(tmp));
	if (l < 0)
		panic("oh shit\n");

	m = lrc_alloc(lrc_message_size(&tmp));
	if (!m)
		panic("Ran out of memory");

	lrc_memcpy(m, &tmp, sizeof(tmp));

	if (m->length)
		l += lrc_read(fd, (char *)m + l, m->length);

	if (l != lrc_message_size(m))
		panic("oh shit\n");

	return m;
}

