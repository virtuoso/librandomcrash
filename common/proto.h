/*
 * librandomcrash message passing protocol
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

#ifndef RANDOMCRASH_PROTO_H
#define RANDOMCRASH_PROTO_H

/* packet types */
enum {
	MT_NOOP = 0,
	MT_HANDSHAKE,
	MT_RESPONSE,
	MT_IMGOOD,
	MT_FORK,
	MT_LOGMSG,
	MT_EXIT,
	MT_BUG,
	MT_NR_TOTAL,
};

/* response codes (?) */
enum {
	RESP_OK = 0,
	RESP_LOCKED,
};

/* message packet format */
struct lrc_message {
	unsigned int	type;
	pid_t		pid;
	struct timeval	timestamp;
	/* payload length: so far only used by logmsg */
	size_t		length;
	union {
		/*
		 * "handshake" is sent from lrc to launcher to
		 * indicate there's a new instance of lrc (new
		 * child process has started; child expects a
		 * "response" packet with a file descriptor
		 * after this
		 */
		struct {
			pid_t		ppid;
			unsigned int	flags;
		} handshake;
		/*
		 * "response" is sent from launcher to lrc in
		 * response to a "handshake"; it is sent with
		 * sendmsg(2) together with a file descriptor
		 * of a socket which is allocated for this
		 * child process
		 */
		struct {
			int	code;
			int	fds[2];
			pid_t	recipient;
		} response;
		/*
		 * "imgood" is sent from lrc to launcher when
		 * a descriptor has been retrieved successfully
		 * from a "response"; no payload
		 */
		/*
		 * "fork" is sent from lrc to launcher when a
		 * fork(2) call returns to the parent process
		 */
		struct {
			pid_t	child;
		} fork;
		/*
		 * "logmsg" is sent from lrc to launcher to transfer
		 * a log message for subsequent logging by the launcher
		 */
		struct {
			int	level;
			char	message[0];
		} logmsg;
		/*
		 * "exit" is sent from lrc to launcher when a
		 * process exits
		 */
		struct {
			int	code;
		} exit;
	} payload;
};

#define lrc_message_size(m)			\
	((m)->length + sizeof(*(m)))

int lrc_message_init(struct lrc_message *m, int type);
void lrc_message_prepare(struct lrc_message *m);
int lrc_message_send(int fd, struct lrc_message *m);
struct lrc_message *lrc_message_recv(int fd);

struct lrc_bus {
	int		fd_in;
	int		fd_out;
	unsigned	connected:1;
};

#endif /* RANDOMCRASH_PROTO_H */
