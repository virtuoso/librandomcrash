#include "override.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <dlfcn.h>

static const char my_name[] = "librandomcrash";
static const char my_ver[] = "0.0.1";

void basic_nastiness(struct override *o)
{
	fprintf(stderr, "--------------------\n");
}

extern struct override *lrc_overrides[];

/*static const struct override overrides[] = {
	{
		.name = "open",
		.new_func = basic_nastiness,
		.nargs = 3,
	}, {
		.name = "close",
		.new_func = basic_nastiness,
		.nargs = 1,
	}
	};*/

void __lrc_call_entry(struct override *o)
{
	fprintf(stderr, "--- %s() called ---\n", o->name);
}

void __ctor lrc_init(void)
{
	int i;

	fprintf(stderr, "%s, %s initializing\n", my_name, my_ver);
	for (i = 0; lrc_overrides[i]; i++) {
		fprintf(stderr, "=> loading %s wrapper\n",
			lrc_overrides[i]->name);
		lrc_overrides[i]->orig_func = dlsym(RTLD_NEXT,
						    lrc_overrides[i]->name);
	}
}

void __dtor lrc_done(void)
{
}

#ifdef __YEAH_RIGHT__
int main (void)
{
        return 0;
}
#endif
