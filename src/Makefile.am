lib_LTLIBRARIES = librandomcrash.la

AM_LDFLAGS = -no-undefined

if HAVE_DARWIN
BUILT_SOURCES = librandomcrash.osx.sym
CLEANFILES = $(BUILT_SOURCES)

librandomcrash.osx.sym:
	grep @SYMBOL_PREFIX@ $(srcdir)/librandomcrash.sym | cut -d";" -f 1 >> librandomcrash.osx.sym

AM_LDFLAGS += -Wl,-exported_symbols_list,$(srcdir)/librandomcrash.osx.sym
else
AM_LDFLAGS += -Wl,--version-script,$(srcdir)/librandomcrash.sym
endif


librandomcrash_la_SOURCES = main.c

librandomcrash_la_LDFLAGS = $(AM_LDFLAGS) \
			    -version-info @LT_VERSION@
