#include <proto/exec.h>
#include <proto/dos.h>
#include "stabs.h"
#include <string.h>

extern long __LIB_LIST__;

/* The minimum version of the libraries opened for the program: it sets it by
   defining, say, long __oslibversion = 39;  Common, so that definition wins. */
__attribute__((common)) long __oslibversion;

extern __attribute__((noreturn)) void exit(int returncode);

#pragma GCC push_options
#pragma GCC optimize ("-O2")

/* Reports every library that failed to open, with a dos.library of its own:
   dos.library may be one of them, at __oslibversion.  Started from Workbench,
   the program has no output to report to, and exits all the same.  */
static __attribute__((noreturn)) __attribute__((noinline)) void __openliberror(void) {
	static char const msg[] = " failed to open\n";
	struct DosLibrary * DOSBase = (struct DosLibrary *)OldOpenLibrary("dos.library");

	if (DOSBase) {
		BPTR out = Output();
		long * l = &__LIB_LIST__ + 1;
		while (out && *l) {
			long * base = l++;
			char const * const name = *(char **) (l++);
			if (*base == -1) {
				Write(out, (APTR)name, strlen(name));
				Write(out, (APTR)msg, sizeof msg - 1);
			}
		}
		CloseLibrary((struct Library *)DOSBase);
	}
	exit(20);
}

/* Whether NAME ends in .resource.  */
static int is_resource(char const * name) {
	static char const suffix[] = ".resource";
	size_t i = sizeof suffix - 1;
	size_t n = strlen(name);

	if (n < i)
		return 0;
	name += n - i;
	while (i--)
		if (name[i] != suffix[i])
			return 0;
	return 1;
}

void __initlibraries(void) {
	long * l = &__LIB_LIST__ + 1;
	long failed = 0;
	/* A base that fails to open keeps its -1: the list ends at the first 0. */
	while (*l) {
		long * base = l++;
		char const * const name = *(char **) (l++);
		long lib = (long)(is_resource(name) ? OpenResource(name) : OpenLibrary(name, __oslibversion));
		if (lib)
			*base = lib;
		else
			failed = 1;
	}
	if (failed)
		__openliberror();

	/* On an FPU, the IEEE maths libraries set the whole FPCR of the task
	 * that opens them: mathieeesingbas to $40 (round to single precision),
	 * mathieeedoubbas to $10 (round toward zero).  A shell's commands also
	 * share its process and inherit the FPCR and FPSR the previous one
	 * left, trap enables and exception flags included.  Start from the
	 * FPU's reset values, which is what C expects (Annex F.8.3): round to
	 * nearest, extended precision, no traps, and no exception flags set.
	 * The FPU instructions are hand-assembled because the base variant is
	 * built for the 68000. */
	if (SysBase->AttnFlags & AFF_68881) {
		__asm__ volatile ("moveq #0,d0\n\t"
				  ".short 0xf200,0x9000\n\t"	/* fmove.l d0,fpcr */
				  ".short 0xf200,0x8800"	/* fmove.l d0,fpsr */
				  ::: "d0");
	}
}

void __exitlibraries(void) {
	long * l = &__LIB_LIST__ + 1;
	while (*l) {
		long * base = l++;
		char const * const name = *(char **) (l++);
		/* *base == -1: it failed to open */
		/* .resource: never closed, exec has no CloseResource() and resources have no Close vector */
		if (*base != -1 && !is_resource(name))
			CloseLibrary((struct Library *)*base);
	}
}

#pragma GCC pop_options

ADD2INIT(__initlibraries, -79);
ADD2EXIT(__exitlibraries, -79);
