/*
 * librandomcrash configuration
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

#include <stdlib.h>
#include <stdbool.h>

#include "override.h"
#include "log.h"
#include "symbols.h"
#include "lrc-libc.h"
#include "conf.h"

static const int delim = ':';

struct config_opt {
	const char	*name;
	unsigned	flags;
	int		(*parse)(struct config_opt *, char *, size_t);
	union {
        char     *val_str;
        long int val_long;
	};
};

static int opt_str_parse(struct config_opt *co, char *str, size_t len)
{
	size_t vlen = lrc_strlen(co->name) + 1;

	if (len < vlen) {
		log_print(LL_OERR, "invalid %.*s option\n", len, str);
		return -1;
	}

	co->val_str = str + lrc_strlen(co->name) + 1;

	return 0;
}

static int opt_bool_parse(struct config_opt *co, char *str, size_t len)
{
	if (len != lrc_strlen(co->name)) {
		log_print(LL_OERR, "invalid %.*s option\n", len, str);
		return -1;
	}

	co->val_long++;

	return 0;
}

static int opt_long_parse(struct config_opt *co, char *str, size_t len)
{
	size_t vlen = lrc_strlen(co->name) + 1;

	if (len < vlen) {
		log_print(LL_OERR, "invalid %.*s option\n", len, str);
		return -1;
	}

	co->val_long = strtoul(str + lrc_strlen(co->name) + 1, NULL, 0);

	return 0;
}

static int opt_logdir_parse(struct config_opt *co, char *str, size_t len)
{
	return opt_str_parse(co, str, len);
}

static int opt_fd_parse(struct config_opt *co, char *str, size_t len)
{
	return opt_long_parse(co, str, len);
}

static int opt_no_crash_parse(struct config_opt *co, char *str, size_t len)
{
	return opt_bool_parse(co, str, len);
}

static int opt_skip_calls_parse(struct config_opt *co, char *str, size_t len)
{
	return opt_long_parse(co, str, len);
}

static struct config_opt opts[] = {
	[CONF_LOGDIR]    = { "logdir",     0, opt_logdir_parse },
	[CONF_NOCRASH]   = { "no-crash",   0, opt_no_crash_parse },
	[CONF_SKIPCALLS] = { "skip-calls", 0, opt_skip_calls_parse },
	[CONF_FD]        = { "fd",         0, opt_fd_parse },
};

char *lrc_conf_str(int what)
{
	return opts[what].val_str;
}

int lrc_conf_long(int what)
{
	return opts[what].val_long;
}

void lrc_configure(void)
{
	char *inbuf, *s, *e;

	inbuf = lrc_getenv(LRC_CONFIG_ENV);
	if (!inbuf || !inbuf[0]) {
		log_print(LL_OINFO, "no config options supplied\n");
		return;
	}

	debug("config string: %s\n", inbuf);

	s = inbuf;
	do {
		int i;

		e = lrc_strchrnul(s, delim);

		for (i = 0; i < arrsz(opts); i++)
			if (!lrc_strncmp(opts[i].name, s, lrc_strlen(opts[i].name))
			    && (*e == delim || !*e)) {
				opts[i].parse(&opts[i], s, e - s);
				break;
			}

		if (i == arrsz(opts) && e != s)
			log_print(LL_OWARN, "unknown option: %.*s\n", e - s, s);

		s = e + 1;
	} while (*e);
}

