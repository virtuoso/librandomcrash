lib_LTLIBRARIES = librandomcrash.la

AM_LDFLAGS = -no-undefined

librandomcrash_la_SOURCES = main.c

librandomcrash_la_LDFLAGS = $(AM_LDFLAGS) \
			    -version-info @LT_VERSION@

if HAVE_DARWIN
BUILT_SOURCES = librandomcrash.osx.sym
CLEANFILES = $(BUILT_SOURCES)

librandomcrash.osx.sym:
	grep @SYMBOL_PREFIX@ $(srcdir)/librandomcrash.sym | cut -d";" -f 1 >> librandomcrash.osx.sym

librandomcrash_la_LDFLAGS += -Wl,-exported_symbols_list,$(srcdir)/librandomcrash.osx.sym
else
librandomcrash_la_LDFLAGS += -Wl,--version-script,$(srcdir)/librandomcrash.sym
endif
