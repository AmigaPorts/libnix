# Tests that run on the build machine with the host compiler.
# -iquote is searched for "quoted" includes only, so libnix's own
# stdio.h/string.h in sources/headers do not shadow the system ones.

CC ?= cc
CFLAGS ?= -O -Wall

all: check

amigapath: amigapath.c ../sources/nix/extra/__amigapath.c ../sources/headers/amigapath.h stabs.h
	$(CC) $(CFLAGS) -iquote . -iquote ../sources/headers -o $@ amigapath.c

check: amigapath
	./amigapath

clean:
	rm -f amigapath

.PHONY: all check clean
