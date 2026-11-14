libnix: a C link library for AmigaOS/m68k
===

**libnix** is a standard C link library for the gcc cross compiler that targets AmigaOS/m68k. It needs no shared library beyond those of AmigaOS, opens the libraries a program uses by itself, and keeps programs small. It relies on the system headers of [newlib](https://github.com/AmigaPorts/newlib-cygwin).

It also builds `libamiga.a`, the replacement for the NDK's amiga.lib: the stubs and `_LVO` symbols of the libraries of NDK 3.2, and support functions such as `CreatePort()` and `DoMethod()`.

## Building

The toolchain builds and installs libnix with `make libnix`, and tests it with `make check-libnix`. To build it alone you need:

- `m68k-amigaos-gcc` and its binutils in `PATH`, with newlib's headers installed;
- [sfdc](https://github.com/AmigaPorts/sfdc) 1.14 or later;
- the SFD files of the NDK (`*_lib.sfd`);
- GNU make and rsync.

Then, in the source directory:

    make PREFIX=/opt/amiga
    make PREFIX=/opt/amiga install

It builds in the current directory. For a build directory of its own, run make there with the Makefile of the sources:

    mkdir build && cd build
    make -f /path/to/libnix/Makefile PREFIX=/opt/amiga
    make -f /path/to/libnix/Makefile PREFIX=/opt/amiga install

`Makefile` is a symlink to `Makefile.m68k-amigaos`. The variables, with their defaults:

| variable | default | |
|---|---|---|
| `PREFIX` | `/opt/amiga` | where the toolchain is installed, and where `install` puts the libraries |
| `SFDC` | `sfdc` | the sfdc to run |
| `SFD_DIR` | `$(PREFIX)/m68k-amigaos/ndk/lib/sfd` | the directory of the NDK's SFD files |
| `CFLAGS` | `-Os -fomit-frame-pointer` | the toolchain adds `-DIXPATHS`, for Unix paths in the file functions |
| `WERROR` | `-Werror` with gcc 16 or later | `WERROR=` keeps warnings as warnings |
| `FAST_MATH` | empty | `FAST_MATH=-ffast-math` builds the math functions with it, which breaks NaN, infinities and signed zeros |

The libraries are built for every multilib of the compiler (`m68k-amigaos-gcc -print-multi-lib`). `install` puts:

- the C libraries and the startup files in `$(PREFIX)/m68k-amigaos/libnix/lib/<multilib>/`;
- `libamiga.a` in `$(PREFIX)/m68k-amigaos/lib/<multilib>/`, where every C library finds it, and the base multilib's `libstubs.a` in `$(PREFIX)/m68k-amigaos/lib/`;
- `mkstub` in `$(PREFIX)/bin`.

It does not install the headers: the toolchain copies `sources/headers` to `$(PREFIX)/m68k-amigaos/libnix/include`.

## Testing

There are host tests, and target tests that run under a simulator with the installed libraries:

    make check
    make check-target SIM=volamos SIMFLAGS="--stack 4K --sanitize"

Both run the Makefile in `test/`; [test/README.md](test/README.md) has the details.

## Threads

There is one build of libnix, whatever the thread model of the compiler. With a compiler that has one (the toolchain's `THREADS=amigaos` or `THREADS=posix`), `make check-target` also runs the thread tests; with the single-task model it reports them as skipped.

## Authors

libnix 3.2 is based on libnix 2.1 by Matthias Fleischer and Gunther Nikl, with the many fixes and additions of Stefan "Bebbo" Franke for [amiga-gcc](https://github.com/bebbo/amiga-gcc).

- Matthias Fleischer fleischr@izfm.uni-stuttgart.de
- Gunther Nikl gnikl@informatik.uni-rostock.de
- Stefan "Bebbo" Franke bebbo@bejy.net
- and many others from the [AmigaDev](https://www.amiga.dev) community

## License

This is Public Domain.
