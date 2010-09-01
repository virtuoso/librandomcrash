CC     = gcc
CFLAGS = -Wall -O0 -g3
LAUNCHER = ../launcher/randomcrash

TESTS    = t_opendir t_readdir

all: $(TESTS) run_tests lrc-libc-tests

run_tests: $(TESTS)
	for t in $(TESTS); do \
		$(LAUNCHER) -l -- $$t && echo OK $$t || echo FAIL $$t; \
		$(LAUNCHER) -l -n -- $$t && echo FAIL $$t NEG || echo OK $$t NEG; \
	done

clean:
	rm -f *.o $(TESTS) lrc-libc

t_opendir: t_opendir.c

t_readdir: t_readdir.c

lrc-libc-tests: lrc-libc
	./lrc-libc

lrc-libc: lrc-libc.c ../src/lrc-libc.h
	$(CC) $< -o $@

.PHONY: all run_tests clean lrc-libc-tests