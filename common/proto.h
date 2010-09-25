#ifndef RANDOMCRASH_PROTO_H
#define RANDOMCRASH_PROTO_H

enum {
	MT_NOOP = 0,
	MT_HANDSHAKE,
	MT_RESPONSE,
	MT_FORK,
};

struct lrc_message {
	unsigned int	type;
	pid_t		pid;
	size_t		length; /* payload length */
	union {
		struct {
			unsigned int	flags;
		} handshake;
		struct {
			int	code;
			char	buf[0];
		} response;
		struct {
			pid_t	child;
		} fork;
	} payload;
};

#define lrc_message_size(m)			\
	((m)->length + sizeof(*(m)))

int lrc_message_init(struct lrc_message *m);
int lrc_message_send(int fd, struct lrc_message *m);
struct lrc_message *lrc_message_recv(int fd);

struct lrc_bus {
	int		fd_in;
	int		fd_out;
	unsigned	connected:1;
};

#endif /* RANDOMCRASH_PROTO_H */
