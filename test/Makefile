# Tests that run on the build machine with the host compiler.
# -iquote is searched for "quoted" includes only, so libnix's own
# stdio.h/string.h in sources/headers do not shadow the system ones.

CC ?= cc
CFLAGS ?= -O -Wall

# Tests that run on the target, built with the cross compiler against the
# sources here (not the installed library) and run under a simulator.
# They are not part of `check` because they need the toolchain and vamos:
#   make check-target
#   make check-target SIM=volamos SIMFLAGS="--stack 4K --sanitize"

CROSS_CC ?= m68k-amigaos-gcc
CROSS_CFLAGS ?= -O2 -fomit-frame-pointer -noixemul
SIM ?= vamos
SIMFLAGS ?= -s 4

all: check

amigapath: amigapath.c ../sources/nix/extra/__amigapath.c ../sources/headers/amigapath.h stabs.h
	$(CC) $(CFLAGS) -iquote . -iquote ../sources/headers -o $@ amigapath.c

check: amigapath
	./amigapath

# swapstack.c is compiled the way Makefile.gcc6 does it and linked ahead
# of libnix, so the module under test is the one in ../sources.
swapstack.o: ../sources/misc/swapstack.c
	$(CROSS_CC) $(CROSS_CFLAGS) -I../sources/headers -fno-builtin -D__libnix__ -c -o $@ $<

stack: stack.c swapstack.o
	$(CROSS_CC) $(CROSS_CFLAGS) -Wall -o $@ stack.c swapstack.o

# The wide printf modules, compiled as libm builds them (with float
# formatting) and linked ahead of libm and libnix.
vwfprintf.o: ../sources/nix/stdio/__vwfprintf_total_size.c
	$(CROSS_CC) $(CROSS_CFLAGS) -I../sources/headers -fno-builtin -D__libnix__ -DFULL_SPECIFIERS -c -o $@ $<

vswprintf.o: ../sources/nix/stdio/vswprintf.c
	$(CROSS_CC) $(CROSS_CFLAGS) -I../sources/headers -fno-builtin -D__libnix__ -DFULL_SPECIFIERS -c -o $@ $<

wprintf: wprintf.c vswprintf.o vwfprintf.o
	$(CROSS_CC) $(CROSS_CFLAGS) -Wall -o $@ wprintf.c vswprintf.o vwfprintf.o -lm

# The soft-float comparisons, compiled as libm builds them and linked
# ahead of libm.
eqdf2.o: ../sources/math/math/__eqdf2.c
	$(CROSS_CC) $(CROSS_CFLAGS) -I../sources/headers -fno-builtin -ffast-math -D__libnix__ -c -o $@ $<

eqsf2.o: ../sources/math/math/__eqsf2.c
	$(CROSS_CC) $(CROSS_CFLAGS) -I../sources/headers -fno-builtin -ffast-math -D__libnix__ -c -o $@ $<

fcmp: fcmp.c eqdf2.o eqsf2.o
	$(CROSS_CC) $(CROSS_CFLAGS) -m68000 -Wall -o $@ fcmp.c eqdf2.o eqsf2.o -lm

check-target: stack wprintf fcmp
	$(SIM) $(SIMFLAGS) ./stack
	$(SIM) $(SIMFLAGS) ./wprintf
	$(SIM) $(SIMFLAGS) ./fcmp

clean:
	rm -f amigapath stack swapstack.o wprintf vswprintf.o vwfprintf.o \
	  fcmp eqdf2.o eqsf2.o

.PHONY: all check check-target clean
