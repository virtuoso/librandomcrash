#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "override.h"
#include "symbols.h"
#include "handlers.h"
#include "memory.h"
#include "lrc-libc.h"
#include "log.h"

static struct open_stat_file {
	char			*name;
	int			count;
	struct open_stat_file	*next;
} list_head, *list_tail = &list_head;

static int open_stat_probe(struct override *o, void *ctxp)
{
	struct __lrc_callctx_open *args = ctxp;

	lrc_callctx_set_acct_handler(args, &open_stat);

	return 0;
}

static struct open_stat_file *file_find(const char *name)
{
	struct open_stat_file *osf;

	for (osf = list_head.next; osf; osf = osf->next)
		if (!lrc_strcmp(name, osf->name))
			return osf;

	return NULL;
}

static int open_stat_exit(struct override *o, void *ctxp, void *retp)
{
	struct __lrc_callctx_open *args = ctxp;
	const char *file = args->file;
	struct open_stat_file *osf;

	log_print(LL_PINFO, "opening \"%s\"\n", file);

	osf = file_find(file);
	if (!osf) {
		list_tail->next = lrc_alloc(sizeof(*list_tail));
		if (!list_tail->next)
			panic("ran out of memory");

		list_tail->next->name = lrc_alloc(lrc_strlen(file));
		if (!list_tail->next->name)
			panic("ran out of memory");

		lrc_strcpy(list_tail->next->name, file);
		osf = list_tail = list_tail->next;
	}

	osf->count++;

	return 0;
}

static void open_stat_fini(void)
{
	struct open_stat_file *osf;

	for (osf = list_head.next; osf; osf = osf->next)
		log_print(LL_PINFO, " file \"%s\", opened %d times\n",
			  osf->name, osf->count);
}

struct handler open_stat = {
	.fn_name	= "open",
	.handler_name	= "open_stat",
	.enabled	= 1,
	.fini_func	= open_stat_fini,
	.probe_func	= open_stat_probe,
	.exit_func	= open_stat_exit,
};

