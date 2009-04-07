lib_LTLIBRARIES = librandomcrash.la

AM_LDFLAGS = -no-undefined

librandomcrash_la_SOURCES = main.c

librandomcrash_la_LDFLAGS = $(AM_LDFLAGS) \
							-version-info @LT_VERSION@ \
						    -Wl,--version-script,$(srcdir)/librandomcrash.sym

