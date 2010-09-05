/*
 * standard library wrapper functions' tests
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
#include <stdio.h>
#include <stdbool.h>

#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <dirent.h>
#include <signal.h>

#include "../src/dummies.c"

/*
 * just print the message, the function that panic()ed should return
 * garbage anyway
 */
void panic(const char *msg)
{
	fputs(msg, stderr);
}

#include "../src/lrc-libc.h"

bool lrc_is_up() { return false; }

#define ARRSZ(x) (sizeof(x) / sizeof(*x))

struct testcase {
	const char	*name;
	int		(*testfn)(void *data);
	void		*data;
};

static const char some_string[]		= "Mashed potatoes";
static const char some_other_string[]	= "Mushroom ravioli";
static const char some_string_sub[]	= "Mashed potato"; /* substring */

static int strcmp_equal_strings(void *data)
{
	return lrc_strcmp(some_string, some_string);
}

static int strcmp_unequal_strings(void *data)
{
	return !lrc_strcmp(some_string, some_other_string);
}

static int strncmp_equal_strings(void *data)
{
	return lrc_strncmp(some_string, some_string, strlen(some_string));
}

static int strncmp_unequal_strings(void *data)
{
	return !lrc_strncmp(some_string, some_other_string,
			    strlen(some_string));
}

static int strncmp_equal_substrings1(void *data)
{
	return lrc_strncmp(some_string, some_string_sub,
			    strlen(some_string_sub));
}

static int strncmp_equal_substrings2(void *data)
{
	return lrc_strncmp(some_string_sub, some_string,
			    strlen(some_string_sub));
}

static struct testcase testcases[] = {
	{
		.name	= "strcmp equal strings",
		.testfn	= strcmp_equal_strings,
	},
	{
		.name	= "strcmp unequal strings",
		.testfn	= strcmp_unequal_strings,
	},
	{
		.name	= "strncmp equal strings",
		.testfn	= strncmp_equal_strings,
	},
	{
		.name	= "strncmp unequal strings",
		.testfn	= strncmp_unequal_strings,
	},
	{
		.name	= "strncmp equal substrings 1",
		.testfn	= strncmp_equal_substrings1,
	},
	{
		.name	= "strncmp equal substrings 2",
		.testfn	= strncmp_equal_substrings2,
	},
};

int main(int argc, char **argv)
{
	int i, ret = 0;

	for (i = 0; i < ARRSZ(testcases); i++) {
		int oneret = testcases[i].testfn(testcases[i].data);

		printf("%s: %s\n", oneret ? "FAIL" : "PASS", testcases[i].name);

		ret += !!oneret;
	}

	return ret;
}
