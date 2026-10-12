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
	python3 run-atan2-zero.py
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

# Compile the implementations with the library's fast-math setting, but
# keep the bit-pattern assertions free of fast-math and compiler builtins.
atan2.o: ../sources/math/trans/atan2.c
	$(CROSS_CC) $(CROSS_CFLAGS) -fno-builtin -ffast-math -c -o $@ $<

atan2f.o: ../sources/math/trans/atan2f.c
	$(CROSS_CC) $(CROSS_CFLAGS) -fno-builtin -ffast-math -c -o $@ $<

atan2-zero: atan2-zero.c atan2.o
	$(CROSS_CC) $(CROSS_CFLAGS) -fno-fast-math -fno-builtin -Wall -o $@ $^ -lm

atan2f-zero: atan2f-zero.c atan2f.o
	$(CROSS_CC) $(CROSS_CFLAGS) -fno-fast-math -fno-builtin -Wall -o $@ $^ -lm

# The soft-float comparisons, compiled as libm builds them and linked
# ahead of libm.
eqdf2.o: ../sources/math/math/__eqdf2.c
	$(CROSS_CC) $(CROSS_CFLAGS) -I../sources/headers -fno-builtin -ffast-math -D__libnix__ -c -o $@ $<

eqsf2.o: ../sources/math/math/__eqsf2.c
	$(CROSS_CC) $(CROSS_CFLAGS) -I../sources/headers -fno-builtin -ffast-math -D__libnix__ -c -o $@ $<

fcmp: fcmp.c eqdf2.o eqsf2.o
	$(CROSS_CC) $(CROSS_CFLAGS) -m68000 -Wall -o $@ fcmp.c eqdf2.o eqsf2.o -lm

# The address parsers, compiled as libsocket builds them. The ctype macros
# warn when passed a plain char, which the run cannot reliably catch: an
# out-of-range lookup reads whatever precedes the table.
INET_CFLAGS = -Wall -Werror=char-subscripts

inet_aton.o: ../sources/socket/arpa/inet_aton.c
	$(CROSS_CC) $(CROSS_CFLAGS) $(INET_CFLAGS) -I../sources/headers -fno-builtin -D__libnix__ -c -o $@ $<

inet_network.o: ../sources/socket/arpa/inet_network.c
	$(CROSS_CC) $(CROSS_CFLAGS) $(INET_CFLAGS) -I../sources/headers -fno-builtin -D__libnix__ -c -o $@ $<

inet: inet.c inet_aton.o inet_network.o
	$(CROSS_CC) $(CROSS_CFLAGS) -Wall -o $@ inet.c inet_aton.o inet_network.o

check-target: stack wprintf fcmp inet atan2-zero atan2f-zero
	$(SIM) $(SIMFLAGS) ./stack
	$(SIM) $(SIMFLAGS) ./wprintf
	$(SIM) $(SIMFLAGS) ./fcmp
	$(SIM) $(SIMFLAGS) ./inet
	$(SIM) $(SIMFLAGS) ./atan2-zero
	$(SIM) $(SIMFLAGS) ./atan2f-zero

clean:
	rm -f amigapath stack swapstack.o wprintf vswprintf.o vwfprintf.o \
	  fcmp eqdf2.o eqsf2.o inet inet_aton.o inet_network.o atan2-zero atan2f-zero atan2.o atan2f.o


.PHONY: all check check-target clean
