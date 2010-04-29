#ifndef RANDOMCRASH_HANDLERS_H
#define RANDOMCRASH_HANDLERS_H

/*
 * a set of callbacks to be run for a certain intercepted call
 * see doc/handlers.txt for a brief introduction
 */
struct handler {
	int	enabled:1;
	/* name of an intercepted call this handler deals with */
	char	*fn_name;
	/* name of this handler, must be unique */
	char	*handler_name;
	/* this will be called upon program exit for all accounting handlers */
	void	(*fini_func)(void);
	/* decide if the handler wants to run */
	int	(*probe_func)(struct override *, void *);
	/* called upon library function entry */
	int	(*entry_func)(struct override *, void *);
	/* called upon library function exit */
	int	(*exit_func)(struct override *, void *, void *);
};

/* all the handlers that we know about */
extern struct handler read_randomcase;
extern struct handler open_stat;
extern struct handler opendir_retnull;
extern struct handler getenv_retnull;
extern struct handler readdir_retnull;

#endif
