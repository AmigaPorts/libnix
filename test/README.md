# libnix tests

Everything is built in `test/build/` (or `O=<dir>`); `make clean` removes it.
Add `V=1` to see the commands.

## Target tests

Built with the cross compiler and run under a simulator:

```
make -C test check-target
make -C test check-target SIM=volamos SIMFLAGS="--stack 4K --sanitize"
```

Each test prints `PASS: <test>` or `FAIL: <test>` followed by its output,
and the run ends with the number of failures. Each test runs in the build
directory, where it may create files.

A test is `<test>.c`, linked ahead of the installed libraries with the
library sources it lists in `<test>_SRCS`, so it tests the sources here,
not the installed libnix. They are compiled as `Makefile.m68k-amigaos`
compiles them (`LIB_CFLAGS`), plus `<test>_LIBFLAGS`; `<test>_CFLAGS` and
`<test>_LIBS` go to the test's own compile and link. To add a test, add its
name to `TARGET_TESTS` and set the variables it needs.

For a hard-float run, use a build directory of its own:

```
make -C test check-target O=build-fpu CROSS_CFLAGS="-O2 -fomit-frame-pointer -noixemul -m68020 -m68881" SIM=volamos SIMFLAGS="--cpu 68020 --fpu --stack 4K --sanitize"
```

## Host tests

```
make -C test check
```

`amigapath` checks the Unix-to-Amiga path conversion. `check-atan2`
compiles the atan2 and atan2f library sources for the host, stubbing only
the Amiga IEEEDPAtan/IEEESPAtan calls, in normal and `-ffast-math` builds.
Each precision has 24 cases: signed-zero pairs, zero y with finite or
infinite x, vertical axes, quadrants, and NaN x with zero y. The assertions
compare IEEE-754 bits and are compiled without fast-math or builtins.

## Manual tests

`make -C test manual` builds the tests the simulators cannot run. Run them
on AmigaOS:

- `popen` needs PIPE: and a shell, see `popen.c`.
- `setlocale_threads` exercises concurrent locale changes. It needs Exec
  tasks, libpthread, and a libnix built by a thread-enabled compiler, so
  that `__posix_threads__` enables the semaphore. It prints
  `setlocale_threads OK`.
- `cxa-dtors` needs a thread-enabled GCC and libstdc++. It checks that
  thread-local destructors run when a thread exits, and that static
  destructors interleave with `atexit()` in reverse registration order. It
  prints `dtors OK`.
