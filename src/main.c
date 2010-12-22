/*
 * librandomcrash core
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

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>

#include "override.h"
#include "handlers.h"
#include "symbols.h"
#include "log.h"
#include "lrc-libc.h"
#include "memory.h"
#include "conf.h"
#include "proto.h"

static const char my_name[] = PACKAGE;
static const char my_ver[] = VERSION;

static __thread int lrc_up;

/*
 * lrc_depth explicitly guards against original libc functions
 * being overridden when called from lrc code
 */
static __thread unsigned int lrc_depth;
static __thread sigset_t lrc_saved_sigset;

/*
 * Block all signals and save previous signal mask
 */
static void lrc_sigblock(void)
{
	sigset_t set;

	lrc_sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, &lrc_saved_sigset);
}

/*
 * Restore signal mask
 */
static void lrc_sigunblock(void)
{
	sigprocmask(SIG_SETMASK, &lrc_saved_sigset, NULL);
	lrc_sigemptyset(&lrc_saved_sigset);
}

/*
 * Indicate that the execution is within lrc
 */
static void lrc_enter(void)
{
	lrc_sigblock();
	lrc_depth++;
	lrc_sigunblock();
}

static bool lrc_try_enter(void)
{
	lrc_sigblock();

	if (lrc_depth) {
		lrc_sigunblock();
		return false;
	}

	lrc_depth++;
	lrc_sigunblock();

	return true;
}

static void lrc_leave(void)
{
	if (!lrc_depth)
		panic("inconsistent lrc depth");

	lrc_depth--;
}

bool lrc_is_up(void)
{
	return !!lrc_up;
}

extern struct override *lrc_overrides[];
extern struct handler *acct_handlers[];
extern struct handler *handlers[];

#define MAXQUEUE 32

struct lrc_bus lrc_bus;

/*
 * Initialize a communication channel between lrc and its launcher
 */
void lrc_initbus(void)
{
	struct lrc_message m;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct iovec iov;
	pid_t dest_pid;
	char buf[CMSG_SPACE(sizeof(int))];
	ssize_t len;

	if (!lrc_conf_long(CONF_FD))
		return;

	lrc_bus.fd_in = lrc_bus.fd_out = lrc_conf_long(CONF_FD);

retry:
	lrc_message_init(&m, MT_HANDSHAKE);
	m.payload.handshake.ppid = getppid();
	m.payload.handshake.flags = 0xabbadead; /* XXX */
	lrc_message_send(lrc_bus.fd_out, &m);

	lrc_memset(&msg, 0, sizeof(msg));
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);
	iov.iov_base = &m;
	iov.iov_len = sizeof(m);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	do {
		len = recvmsg(lrc_bus.fd_in, &msg, 0);
	} while (len == -1 && errno == EINTR);

	if (len == -1) {
		log_print(LL_OERR, "recvmsg failed: %m\n");
		panic("Can't receive ipc socket\n");
	}

	if (m.type == MT_RESPONSE) {
		int fd = *(int *)CMSG_DATA(cmsg);

		if (m.payload.response.recipient != lrc_gettid()) {
			log_print(LL_OINFO, "### wrong rcpt %d\n",
				  m.payload.response.recipient);
			goto retry;
		}
		log_print(LL_OINFO, "connected to launcher, fds: %d<>%d\n",
			  m.payload.response.fds[0], m.payload.response.fds[1]);

		cmsg = CMSG_FIRSTHDR(&msg);
		fd = *(int *)CMSG_DATA(cmsg);

		lrc_bus.fd_in = lrc_bus.fd_out = fd;
		lrc_bus.connected = 1;
		lrc_message_init(&m, MT_IMGOOD);
		lrc_message_send(lrc_bus.fd_out, &m);
	}
}

/*
 * Tell our launcher that we're exiting
 */
static void exit_notify(int status, void *data)
{
	struct lrc_message m;

	lrc_enter();
	lrc_message_init(&m, MT_EXIT);
	m.payload.exit.code = status;
	lrc_message_send(lrc_bus.fd_out, &m);
	log_print(LL_OINFO, "STILL ALIVE\n");
	lrc_leave();
}

static unsigned long int lrc_callno = 0;

int lrc_call_entry(struct override *o, void *ctxp)
{
	struct handler *queue[MAXQUEUE];
	int i, qlast = 0, ret = 0;

	if (!lrc_try_enter())
		return 0;

	if (!lrc_up)
		panic("lrc is not up where it ought to be.\n");

	lrc_callno++;

	debug("%s() entry, call nr: %lu\n", o->name, lrc_callno);

	/* first, run the first enabled accounting handler for this call */
	/* these should move to the launcher */
	for (i = 0; acct_handlers[i]; i++)
		if (!lrc_strcmp(acct_handlers[i]->fn_name, o->name) &&
		    acct_handlers[i]->enabled &&
		    acct_handlers[i]->probe_func &&
		    !acct_handlers[i]->probe_func(o, ctxp))
			break;

	if (!lrc_strcmp(o->name, "_exit") ||
	    !lrc_strcmp(o->name, "_Exit") ||
	    !lrc_strcmp(o->name, "exit")) {
		struct __lrc_callctx__exit *args = ctxp;
		exit_notify(args->status, NULL);
	}

	if (!lrc_conf_long(CONF_NOCRASH)) {
		if (lrc_callno <= lrc_conf_long(CONF_SKIPCALLS)) {
			debug("%s() skipping\n", o->name);
			goto out;
		}

		for (i = 0; handlers[i] && qlast < MAXQUEUE; i++)
			if (!lrc_strcmp(handlers[i]->fn_name, o->name) &&
			    handlers[i]->enabled &&
			    handlers[i]->probe_func &&
			    !handlers[i]->probe_func(o, ctxp))
				queue[qlast++] = handlers[i];

		if (!qlast) {
			warn("no handlers for %s call\n", o->name);
			goto out;
		}

		/* XXX: need to pick a random one */
		ret = queue[0]->entry_func(o, ctxp);
	}

out:
	lrc_leave();

	return ret;
}

void lrc_call_exit(struct override *o, void *ctxp, void *retp)
{
	struct lrcpriv_callctx *callctx =
		&((struct __lrc_callctx *)ctxp)->callctx;

	if (!lrc_try_enter())
		return;

	debug("%s() exit, ret=%d\n", o->name,
		  retp ? *(int *)retp : 0);

	/* fork is a special case */
	if (!lrc_strcmp(o->name, "fork") && *(pid_t *)retp > 0) {
		struct lrc_message m;

		lrc_message_init(&m, MT_FORK);
		m.payload.fork.child = *(pid_t *)retp;
		lrc_message_send(lrc_bus.fd_out, &m);
	} else if (!lrc_strcmp(o->name, "fork") && *(pid_t *)retp == 0) {
		lrc_initbus();
	}

	if (callctx->acct_handler && callctx->acct_handler->exit_func)
		callctx->acct_handler->exit_func(o, ctxp, retp);

	if (callctx->handler && callctx->handler->exit_func)
		callctx->handler->exit_func(o, ctxp, retp);
	lrc_leave();
}

void __ctor lrc_init(void);

EXPORT void __ctor lrc_init(void)
{
	int i;

	if (lrc_up)
		return;

	/* we can't actually do anything before we do this */
	for (i = 0; lrc_overrides[i]; i++)
		lrc_overrides[i]->orig_func = dlsym(RTLD_NEXT,
						    lrc_overrides[i]->name);

	lrc_sigemptyset(&lrc_saved_sigset);

	lrc_enter();

	log_init();
	log_print(LL_OINFO, "%s, %s initialized\n", my_name, my_ver);

	lrc_initmem();
	lrc_configure();

	lrc_initbus();

	on_exit(exit_notify, NULL);

	lrc_up++;

	lrc_leave();
}

EXPORT void __dtor lrc_done(void)
{
	int i;

	for (i = 0; acct_handlers[i]; i++)
		if (acct_handlers[i]->fini_func)
			acct_handlers[i]->fini_func();
	debug("%s: STILL ALIVE\n", __func__);
}

#ifdef __YEAH_RIGHT__
int main(void)
{
        return 0;
}
#endif
