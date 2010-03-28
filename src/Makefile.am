lib_LTLIBRARIES = librandomcrash.la
bin_PROGRAMS = lrc_test

CLEANFILES =

EXTRA_DIST = librandomcrash.sym \
	librandomcrash.pc.in

AM_LDFLAGS = -no-undefined

librandomcrash_la_SOURCES = symbols.c main.c read.c opendir.c handlers.c \
	log.c memory.c \
	stat/open.c

librandomcrash_la_LIBADD = -ldl

lrc_test_SOURCES = symbols.c main.c read.c opendir.c handlers.c \
	log.c memory.c \
	stat/open.c

lrc_test_LDADD = -ldl
lrc_test_CPPFLAGS = -D__YEAH_RIGHT__

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


