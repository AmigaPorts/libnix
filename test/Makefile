# libnix's tests:
#   make check                 host tests, with the host compiler
#   make check-target          target tests, under a simulator
#   make check-target SIM=volamos SIMFLAGS="--stack 4K --sanitize"
#   make manual                tests to run by hand on AmigaOS
#   make build/<test>          one test
# Everything is built in $(O); V=1 shows the commands.

CC ?= cc
CFLAGS ?= -O -Wall

CROSS_CC ?= m68k-amigaos-gcc
CROSS_CXX ?= $(patsubst %gcc,%g++,$(CROSS_CC))
CROSS_CFLAGS ?= -O2 -fomit-frame-pointer -noixemul
SIM ?= vamos
SIMFLAGS ?= -s 4

O ?= build
Q = $(if $(V),,@)

all: check

# ---------------------------------------------------------------------------
# Target tests. Each one is <test>.c, linked ahead of the installed libraries
# with the library sources it names in <test>_SRCS, which are compiled as
# Makefile.m68k-amigaos compiles them, plus <test>_LIBFLAGS. <test>_CFLAGS
# and <test>_LIBS go to the test's own compile and link.

TARGET_TESTS = \
	atan2-zero \
	atan2f-zero \
	c99math \
	fcmp \
	fma \
	fread \
	frexpl \
	fstat \
	ftruncate \
	ftw \
	inet \
	mathfp \
	printf \
	romtag \
	sscanf \
	stack \
	strto \
	sysinfo \
	vfprintf_test \
	vfwprintf_test \
	wprintf

# Need what the simulators lack: PIPE: and a shell (popen), Exec tasks
# (setlocale_threads), a thread-enabled gcc (setlocale_threads, cxa-dtors).
MANUAL_TESTS = \
	cxa-dtors \
	popen \
	setlocale_threads

# The library's flags, FAST_MATH as in Makefile.m68k-amigaos.
FAST_MATH ?=
LIB_CFLAGS = -I../sources/headers -fno-builtin $(FAST_MATH) -D__libnix__ -fno-tree-loop-distribute-patterns

# The signed-zero assertions compare bits, so the tests themselves are built
# without fast-math and builtins.
atan2-zero_SRCS = ../sources/math/trans/atan2.c
atan2-zero_CFLAGS = -fno-fast-math -fno-builtin
atan2-zero_LIBS = -lm

atan2f-zero_SRCS = ../sources/math/trans/atan2f.c
atan2f-zero_CFLAGS = -fno-fast-math -fno-builtin
atan2f-zero_LIBS = -lm

c99math_SRCS = \
	$(addprefix ../sources/math/ldouble/, \
	  acoshl.c asinhl.c atanhl.c cbrtl.c copysignl.c erfcl.c erfl.c exp2l.c \
	  expm1l.c fdiml.c fmal.c fmaxl.c fminl.c ilogbl.c lgammal.c llrintl.c \
	  llroundl.c log1pl.c log2l.c logbl.c lrintl.c lroundl.c nanl.c \
	  nearbyintl.c nextafterl.c nexttowardl.c remainderl.c remquol.c rintl.c \
	  roundl.c scalblnl.c scalbnl.c tgammal.c truncl.c) \
	$(addprefix ../sources/math/mathfp/, \
	  exp2.c exp2f.c lgamma.c lgammaf.c log2f.c signgam.c tgamma.c tgammaf.c) \
	$(addprefix ../sources/math/common/, \
	  nexttoward.c nexttowardf.c)
c99math_LIBFLAGS = -I../sources/math/mathfp
c99math_CFLAGS = -fno-builtin
c99math_LIBS = -lm

fcmp_SRCS = ../sources/math/math/__eqdf2.c ../sources/math/math/__eqsf2.c
fcmp_CFLAGS = -m68000
fcmp_LIBS = -lm

# Built with an FPU (-m68020 -m68881), it also checks the directed rounding modes.
fma_SRCS = ../sources/math/common/s_fma.c ../sources/math/common/sf_fma.c
fma_LIBFLAGS = -I../sources/math/mathfp
fma_CFLAGS = -fno-builtin
fma_LIBS = -lm

fread_SRCS = ../sources/nix/stdio/fread.c ../sources/nix/stdio/__srget.c

frexpl_SRCS = ../sources/math/extra/frexpl.c
frexpl_CFLAGS = -fno-fast-math -fno-builtin
frexpl_LIBS = -lm

fstat_SRCS = ../sources/nix20/stdio/fstat.c

ftruncate_SRCS = ../sources/nix20/stdio/ftruncate.c

ftw_SRCS = ../sources/nix/extra/ftw.c ../sources/nix/extra/nftw.c
ftw_CFLAGS = -I../sources/headers

# The ctype macros warn when passed a plain char, which a run cannot reliably
# catch: an out-of-range lookup reads whatever precedes the table.
inet_SRCS = ../sources/socket/arpa/inet_aton.c ../sources/socket/arpa/inet_network.c
inet_LIBFLAGS = -Wall -Werror=char-subscripts

mathfp_SRCS = $(addprefix ../sources/math/mathfp/,fmod.c fmodf.c frexp.c ldexp.c fabs.c fabsf.c)
mathfp_CFLAGS = -fno-builtin
mathfp_LIBS = -lm

# libm builds the printf family with float formatting.
printf_SRCS = ../sources/nix/stdio/__vfprintf_total_size.c ../sources/nix/stdio/vsprintf.c ../sources/math/stdio/ami_printf_math.c
printf_LIBFLAGS = -DFULL_SPECIFIERS
printf_LIBS = -lm

# The startup code of -ramiga-lib, linked into a program.
romtag_SRCS = ../sources/startup/libinit.c
romtag_CFLAGS = -I../sources/headers

# bebbo's self-tests of the narrow and wide printf engines.
vfprintf_test_SRCS = ../sources/nix/stdio/__vfprintf_total_size.c ../sources/math/stdio/ami_printf_math.c
vfprintf_test_LIBFLAGS = -DFULL_SPECIFIERS
vfprintf_test_CFLAGS = -I../sources/headers -DFULL_SPECIFIERS
vfprintf_test_LIBS = -lm

vfwprintf_test_SRCS = $(addprefix ../sources/nix/stdio/,__vfwprintf_total_size.c vswprintf.c vfwprintf.c wprintf.c) ../sources/math/stdio/ami_printf_math.c
vfwprintf_test_LIBFLAGS = -DFULL_SPECIFIERS
vfwprintf_test_CFLAGS = -I../sources/headers -DFULL_SPECIFIERS
vfwprintf_test_LIBS = -lm

sscanf_SRCS = $(addprefix ../sources/nix/stdio/,sscanf.c vsscanf.c vfscanf.c __srget.c)

stack_SRCS = ../sources/misc/swapstack.c

strto_SRCS = $(addprefix ../sources/nix/stdlib/,strtol.c strtoul.c strtoll.c strtoull.c wcstol.c)
strto_CFLAGS = -fno-builtin

sysinfo_SRCS = ../sources/nix20/misc/uname.c ../sources/nix/extra/getpid.c ../sources/nix20/stdlib/kill.c ../sources/nix/extra/sysconf.c

wprintf_SRCS = $(addprefix ../sources/nix/stdio/,__vfwprintf_total_size.c vswprintf.c vfwprintf.c wprintf.c) ../sources/math/stdio/ami_printf_math.c
wprintf_LIBFLAGS = -DFULL_SPECIFIERS
wprintf_LIBS = -lm

popen_SRCS = ../sources/nix20/stdio/popen.c

setlocale_threads_LIBS = -lpthread -lm

cxa-dtors_LIBS = -lpthread -latomic

# A test's objects go in $(O)/obj/<test>/, so two tests may compile the same
# source with different flags.
test_objs = $(addprefix $(O)/obj/$(1)/,$(notdir $($(1)_SRCS:.c=.o)))

define lib_object
$(O)/obj/$(1)/$(notdir $(2:.c=.o)): $(2)
	@mkdir -p $$(@D)
	$$(Q)$$(CROSS_CC) $$(CROSS_CFLAGS) $$(LIB_CFLAGS) $$($(1)_LIBFLAGS) -MMD -MP -c -o $$@ $$<
endef

define test_program
$(foreach src,$($(1)_SRCS),$(eval $(call lib_object,$(1),$(src))))
$(O)/$(1): $(wildcard $(1).c) $(call test_objs,$(1))
	@mkdir -p $$(@D)
	$$(Q)$$(CROSS_CC) $$(CROSS_CFLAGS) -Wall $$($(1)_CFLAGS) -o $$@ $$^ $$($(1)_LIBS)
endef

$(foreach t,$(filter-out cxa-dtors,$(TARGET_TESTS) $(MANUAL_TESTS)),$(eval $(call test_program,$(t))))

$(O)/cxa-dtors: cxa-dtors.cpp
	@mkdir -p $(@D)
	$(Q)$(CROSS_CXX) $(CROSS_CFLAGS) -fuse-cxa-atexit -std=gnu++23 -o $@ $< $(cxa-dtors_LIBS)

-include $(wildcard $(O)/obj/*/*.d)

# Each test runs in $(O), where it may create files, and prints its output
# only when it fails.
check-target: $(addprefix $(O)/,$(TARGET_TESTS))
	$(Q)failed=0; \
	for t in $(TARGET_TESTS); do \
		if (cd $(O) && $(SIM) $(SIMFLAGS) ./$$t) >$(O)/$$t.log 2>&1; then \
			echo "PASS: $$t"; \
		else \
			echo "FAIL: $$t"; sed 's/^/    /' $(O)/$$t.log; failed=$$((failed + 1)); \
		fi; \
	done; \
	echo "$$failed of $(words $(TARGET_TESTS)) target tests failed"; \
	test $$failed = 0

manual: $(addprefix $(O)/,$(MANUAL_TESTS))

# ---------------------------------------------------------------------------
# Host tests. -iquote is searched for "quoted" includes only, so libnix's own
# stdio.h/string.h in sources/headers do not shadow the system ones.

$(O)/host/amigapath: amigapath.c ../sources/nix/extra/__amigapath.c ../sources/headers/amigapath.h stabs.h
	@mkdir -p $(@D)
	$(Q)$(CC) $(CFLAGS) -iquote . -iquote ../sources/headers -o $@ amigapath.c

# atan2 and atan2f, normal and -ffast-math, with the Amiga IEEE calls stubbed.
HOST_MATH_CFLAGS = $(CFLAGS) -std=c99 -fno-builtin -fno-fast-math
HOST_ATAN_TESTS = $(addprefix $(O)/host/,atan2-zero atan2-zero-fast atan2f-zero atan2f-zero-fast)
HOST_ATAN_HEADERS = host/proto/mathieeesingtrans.h host/proto/mathieeedoubtrans.h

$(O)/host/atan-stub.o: host/atan-stub.c $(HOST_ATAN_HEADERS)
	@mkdir -p $(@D)
	$(Q)$(CC) $(HOST_MATH_CFLAGS) -c -o $@ $<

$(O)/host/%-test.o: %-zero.c
	@mkdir -p $(@D)
	$(Q)$(CC) $(HOST_MATH_CFLAGS) -DLIBNIX_HOST_TEST -c -o $@ $<

$(O)/host/%-normal.o: ../sources/math/trans/%.c $(HOST_ATAN_HEADERS)
	@mkdir -p $(@D)
	$(Q)$(CC) $(HOST_MATH_CFLAGS) -D$*=libnix_$* -Ihost -c -o $@ $<

$(O)/host/%-fast.o: ../sources/math/trans/%.c $(HOST_ATAN_HEADERS)
	@mkdir -p $(@D)
	$(Q)$(CC) $(HOST_MATH_CFLAGS) -ffast-math -D$*=libnix_$* -Ihost -c -o $@ $<

$(O)/host/%-zero: $(O)/host/%-test.o $(O)/host/%-normal.o $(O)/host/atan-stub.o
	$(Q)$(CC) $(HOST_MATH_CFLAGS) $(LDFLAGS) -o $@ $^ -lm $(LDLIBS)

$(O)/host/%-zero-fast: $(O)/host/%-test.o $(O)/host/%-fast.o $(O)/host/atan-stub.o
	$(Q)$(CC) $(HOST_MATH_CFLAGS) $(LDFLAGS) -o $@ $^ -lm $(LDLIBS)

check-atan2: $(HOST_ATAN_TESTS)
	$(Q)for t in $^; do ./$$t || exit 1; done

check: $(O)/host/amigapath check-atan2
	$(Q)./$(O)/host/amigapath

clean:
	rm -rf $(O)

.PHONY: all check check-atan2 check-target manual clean
.SECONDARY:
