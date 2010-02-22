#include <stdlib.h>
#include "handlers.h"

struct handler *acct_handlers[] = {
	&open_stat,
	NULL,
};

struct handler *handlers[] = {
	&read_randomcase,
	&opendir_retnull,
	NULL,
};

