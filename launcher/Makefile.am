bin_PROGRAMS = randomcrash

AM_LDFLAGS = -no-undefined

randomcrash_SOURCES = \
	randomcrash.c \
	bus.c

randomcrash_CPPFLAGS = -I../common -DPKG_PREFIX=\"$(prefix)\" $(debug)
randomcrash_CPPFLAGS = -DPKG_PREFIX=\"$(prefix)\" $(debug) -DLRC_SRC_BASE_DIR=\"$(abs_top_builddir)\"

