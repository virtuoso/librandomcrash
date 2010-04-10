lib_LTLIBRARIES = librandomcrash.la
bin_PROGRAMS = lrc_test randomcrash

CLEANFILES =

EXTRA_DIST = librandomcrash.sym \
	librandomcrash.pc.in

AM_LDFLAGS = -no-undefined

randomcrash_SOURCES = randomcrash.c
randomcrash_CPPFLAGS = -DPKG_PREFIX=\"$(prefix)\" $(debug)

librandomcrash_la_SOURCES = symbols.c main.c read.c opendir.c handlers.c \
	getenv.c \
	log.c memory.c conf.c \
	stat/open.c

librandomcrash_la_LIBADD = -ldl
librandomcrash_la_CPPFLAGS = $(debug)

lrc_test_SOURCES = symbols.c main.c read.c opendir.c handlers.c \
	getenv.c \
	log.c memory.c conf.c \
	stat/open.c

lrc_test_LDADD = -ldl
lrc_test_CPPFLAGS = -D__YEAH_RIGHT__  $(debug)

symbols.c: symbols.list
	awk -f symbols.awk < $< > $@

librandomcrash_la_LDFLAGS = $(AM_LDFLAGS) \
			    -version-info @LT_VERSION@

if HAVE_DARWIN
BUILT_SOURCES = librandomcrash.osx.sym
CLEANFILES += $(BUILT_SOURCES)

librandomcrash.osx.sym:
	grep @SYMBOL_PREFIX@ $(srcdir)/librandomcrash.sym | cut -d";" -f 1 >> librandomcrash.osx.sym

librandomcrash_la_LDFLAGS += -Wl,-exported_symbols_list,$(srcdir)/librandomcrash.osx.sym
else
librandomcrash_la_LDFLAGS += -Wl,--version-script,$(srcdir)/librandomcrash.sym
endif

###############################################################################
# pkg-config integration
###############################################################################

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = librandomcrash.pc
CLEANFILES += $(pkgconfig_DATA)

.pc.in.pc: $(top_builddir)/config.status
	cd "$(top_builddir)" && \
	$(SHELL) ./config.status --file="src/$@"


