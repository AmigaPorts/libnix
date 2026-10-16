/* Stand-in for the toolchain's stabs.h when a libnix source is compiled
   on the build machine for a test: no constructor list there.  */
#define ADD2INIT(func, pri)
#define ADD2EXIT(func, pri)
