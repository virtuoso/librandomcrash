bin_PROGRAMS = randomcrash

AM_LDFLAGS = -no-undefined

randomcrash_SOURCES = \
	randomcrash.c \
	util.c        \
	bus.c

randomcrash_CPPFLAGS = -DPKG_PREFIX=\"$(prefix)\" $(debug) -DLRC_SRC_BASE_DIR=\"$(abs_top_builddir)\" -I../common

