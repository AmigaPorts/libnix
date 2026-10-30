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

#define FAILED " failed to load\n"

static __attribute__((noreturn)) __attribute__((noinline)) void __openliberror() {
	char buf[80];

	long * l = &__LIB_LIST__ + 1;
	while (*l) {
		long * base = l++;
		char const * const name = *(char **) (l++);
		if (*base == -1) {
			char * p = buf;
			for (char const * q = name; *q;) { *p++ = *q++; } // strcpy
			for (char const * q = FAILED; *q;) { *p++ = *q++; } // strcat
			*p = 0;
			Write(Output(), buf, strlen(buf));
		}
	}
	exit(20);
}

__attribute__((noinline))
static int endswithResource(char const * name) {
	char const * p = name;
	while (*p) ++p;
	p -= 9;
	if (p < name) return 0;
	for (char const * q = ".resource";*q;++p, ++q) {
		if (*p != *q)
			return 0;
	}
	return 1;
}

void __initlibraries(void) {
	long * l = &__LIB_LIST__ + 1;
	long failed = 0;
	while (*l) {
		long * base = l++;
		char const * const name = *(char **) (l++);
		long lib = (long)(endswithResource(name) ? OpenResource(name) : OpenLibrary(name, __oslibversion));
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
		if (*base != -1) {
			if (!endswithResource(name))
				CloseLibrary((struct Library *)*base);
		}
	}
}

#pragma GCC pop_options

ADD2INIT(__initlibraries, -79);
ADD2EXIT(__exitlibraries, -79);
