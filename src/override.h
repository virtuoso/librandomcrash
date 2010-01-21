#ifndef RANDOMCRASH_OVERRIDE_H
#define RANDOMCRASH_OVERRIDE_H

typedef void (*fnptr_t)(void);

struct override {
	const char *name;
	fnptr_t orig_func;
	fnptr_t new_func;
	int nargs;
	unsigned int flags;
};

#define arrsz(__a) (sizeof(__a) / sizeof(__a[0]))

#define __ctor __attribute__((constructor))
#define __dtor __attribute__((destructor))

#endif
