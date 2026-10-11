# atan2f signed-zero regression

Run the actual `sources/math/trans/atan2f.c` on a host, using a stub for
`IEEESPAtan`:

```
python3 tests/run-atan2f-zero.py
```

The runner tests both ordinary compilation and `-ffast-math`, which is used by
`Makefile.gcc6`. The test itself is compiled without fast-math and compares
IEEE-754 bit patterns so negative zero cannot be mistaken for positive zero.
It covers all four signed-zero pairs, zero `y` with positive/negative finite
and infinite `x`, vertical axes, the four quadrants, and NaN `x` with zero `y`.

To test the installed Amiga library (disable compiler builtins):

```
m68k-amigaos-gcc -O2 -fno-builtin -noixemul tests/atan2f-zero.c -lm -o atan2f-zero
vamos -q atan2f-zero
```

To test the patched source before installing the rebuilt library:

```
m68k-amigaos-gcc -Os -fomit-frame-pointer -fno-builtin -ffast-math -noixemul \
  -c sources/math/trans/atan2f.c -o atan2f.o
m68k-amigaos-gcc -O2 -fno-builtin -noixemul tests/atan2f-zero.c atan2f.o \
  -lm -o atan2f-zero
vamos -q atan2f-zero
```

For hard-float, add `-m68060 -mhard-float` to both compiler commands and use
`vamos -q -C 68040 atan2f-zero` (or run on an Amiga with an FPU).
The expected final line is `atan2f signed-zero: PASS (24 cases)`.
