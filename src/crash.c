#include "override.h"
#include "log.h"

void __noret lrc_going_to_crash(const char *site, const char *cond)
{
	log_print(LL_PERR, "function %s call would lead to a crash "
		  "due to \"%s\".\n", site, cond);
	panic("going to crash\n");
}
