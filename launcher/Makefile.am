bin_PROGRAMS = randomcrash

AM_CFLAGS = \
	-std=c99 -Wall -Wundef -Wstrict-prototypes \
	-Werror-implicit-function-declaration \
	-Wmissing-declarations
AM_LDFLAGS = -no-undefined

randomcrash_SOURCES = \
	randomcrash.c \
	util.c        \
	maps.c	      \
	bus.c         \
	child.c	      \
	backtrace.c

randomcrash_CPPFLAGS = -DPKG_PREFIX=\"$(prefix)\" $(debug) -DLRC_SRC_BASE_DIR=\"$(abs_top_builddir)\" -I../common

