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

# Host-only names keep these objects separate from the target builds.
HOST_MATH_CFLAGS = $(CFLAGS) -std=c99 -fno-builtin -fno-fast-math
HOST_ATAN_TESTS = host-atan2-zero host-atan2-zero-fast host-atan2f-zero host-atan2f-zero-fast
HOST_ATAN_HEADERS = host/proto/mathieeesingtrans.h host/proto/mathieeedoubtrans.h

host-atan-stub.o: host/atan-stub.c $(HOST_ATAN_HEADERS)
	$(CC) $(HOST_MATH_CFLAGS) -c -o $@ $<

host-%-test.o: %-zero.c
	$(CC) $(HOST_MATH_CFLAGS) -DLIBNIX_HOST_TEST -c -o $@ $<

host-%-normal.o: ../sources/math/trans/%.c $(HOST_ATAN_HEADERS)
	$(CC) $(HOST_MATH_CFLAGS) -D$*=libnix_$* -Ihost -c -o $@ $<

host-%-fast.o: ../sources/math/trans/%.c $(HOST_ATAN_HEADERS)
	$(CC) $(HOST_MATH_CFLAGS) -ffast-math -D$*=libnix_$* -Ihost -c -o $@ $<

host-%-zero: host-%-test.o host-%-normal.o host-atan-stub.o
	$(CC) $(HOST_MATH_CFLAGS) $(LDFLAGS) -o $@ $^ -lm $(LDLIBS)

host-%-zero-fast: host-%-test.o host-%-fast.o host-atan-stub.o
	$(CC) $(HOST_MATH_CFLAGS) $(LDFLAGS) -o $@ $^ -lm $(LDLIBS)

check-atan2: $(HOST_ATAN_TESTS)
	./host-atan2-zero
	./host-atan2-zero-fast
	./host-atan2f-zero
	./host-atan2f-zero-fast

check: amigapath check-atan2
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

frexpl.o: ../sources/math/extra/frexpl.c
	$(CROSS_CC) $(CROSS_CFLAGS) -fno-builtin -ffast-math -c -o $@ $<

frexpl: frexpl.c frexpl.o
	$(CROSS_CC) $(CROSS_CFLAGS) -fno-fast-math -fno-builtin -Wall -o $@ $^

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

# The directory walkers, compiled against the libnix headers and sources.
ftw.o: ../sources/nix/extra/ftw.c ../sources/headers/ftw.h
	$(CROSS_CC) $(CROSS_CFLAGS) -I../sources/headers -fno-builtin -D__libnix__ -c -o $@ $<

nftw.o: ../sources/nix/extra/nftw.c ../sources/headers/ftw.h
	$(CROSS_CC) $(CROSS_CFLAGS) -I../sources/headers -fno-builtin -D__libnix__ -c -o $@ $<

ftw: ftw.c ftw.o nftw.o
	$(CROSS_CC) $(CROSS_CFLAGS) -I../sources/headers -Wall -o $@ ftw.c ftw.o nftw.o

# Needs PIPE: and a shell, which the simulators lack: build it with
# `make popen` and run it on AmigaOS, see popen.c.
popen.o: ../sources/nix20/stdio/popen.c
	$(CROSS_CC) $(CROSS_CFLAGS) -I../sources/headers -fno-builtin -D__libnix__ -c -o $@ $<

popen: popen.c popen.o
	$(CROSS_CC) $(CROSS_CFLAGS) -Wall -o $@ popen.c popen.o

check-target: stack wprintf fcmp inet atan2-zero atan2f-zero ftw frexpl
	$(SIM) $(SIMFLAGS) ./stack
	$(SIM) $(SIMFLAGS) ./wprintf
	$(SIM) $(SIMFLAGS) ./fcmp
	$(SIM) $(SIMFLAGS) ./inet
	$(SIM) $(SIMFLAGS) ./atan2-zero
	$(SIM) $(SIMFLAGS) ./atan2f-zero
	$(SIM) $(SIMFLAGS) ./ftw
	$(SIM) $(SIMFLAGS) ./frexpl

# Exec tasks are required; vamos cannot run this pthread test.
setlocale_threads: setlocale_threads.c
	$(CROSS_CC) $(CROSS_CFLAGS) -Wall -o $@ $< -lpthread -lm

cxa-dtors: cxa-dtors.cpp
	m68k-amigaos-g++ $(CROSS_CFLAGS) -fuse-cxa-atexit -std=gnu++23 -o $@ $< -lpthread -latomic

clean:
	rm -f $(HOST_ATAN_TESTS) host-atan-stub.o host-atan2-test.o host-atan2f-test.o host-atan2-normal.o host-atan2f-normal.o host-atan2-fast.o host-atan2f-fast.o
	rm -f amigapath stack swapstack.o setlocale_threads cxa-dtors wprintf vswprintf.o vwfprintf.o \
	  fcmp eqdf2.o eqsf2.o inet inet_aton.o inet_network.o atan2-zero atan2f-zero atan2.o atan2f.o \
	  ftw ftw.o nftw.o popen popen.o frexpl frexpl.o



.PHONY: all check check-atan2 check-target clean
