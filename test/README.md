# atan2 and atan2f signed-zero regression

Run the host tests with `make -C test check`, or just the math regressions:

```
make -C test check-atan2
```

The Makefile compiles both actual library sources, stubbing only the Amiga
IEEEDPAtan/IEEESPAtan calls. It checks normal and `-ffast-math` builds.
Each precision has 24 cases: signed-zero pairs, zero y with finite/infinite
x, vertical axes, quadrants, and NaN x with zero y. Assertions compare
IEEE-754 bits and are compiled without fast-math or compiler builtins.

Run the target suite against the patched sources:

```
make -C test check-target
make -C test check-target SIM=volamos SIMFLAGS="--stack 4K --sanitize"
```

The math objects are linked before installed libm and compiled with
`-ffast-math`, as in Makefile.m68k-amigaos. Cross compiler and simulator settings
use the existing CROSS_CC, CROSS_CFLAGS, SIM, and SIMFLAGS variables.
For a hard-float run, clean first and use:

```
make -C test clean
make -C test check-target CROSS_CFLAGS="-O2 -fomit-frame-pointer -noixemul -m68060 -mhard-float" SIMFLAGS="-s 4 -C 68040"
```

Both math tests should report PASS (24 cases).

Build `make -C test setlocale_threads` and run the resulting program on
AmigaOS to exercise concurrent locale changes. It requires Exec tasks,
libpthread, and a libnix built by a thread-enabled compiler so
`__posix_threads__` enables the semaphore; it
should print `setlocale_threads OK`.

Build `make -C test cxa-dtors` with a thread-enabled GCC/libstdc++ and
run it on AmigaOS. It checks that thread-local destructors run when a
thread exits and that static destructors interleave with `atexit()` in
reverse registration order. The test explicitly uses `-fuse-cxa-atexit`
and should print `dtors OK`.
