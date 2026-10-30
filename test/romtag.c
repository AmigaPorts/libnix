/* libinit.o, linked into a program: its function table starts with the four
   standard vectors, and it keeps the romtag that exec finds the library by. */

#include <exec/resident.h>
#include <stdio.h>
#include "libinit.h"

const char LibName[] = "test.library";
const char LibIdString[] = "test.library 1.0";
const UWORD LibVersion = 1;
const UWORD LibRevision = 0;

LONG __stdargs __UserLibInit(struct Library *lib, REG(a4, APTR dataseg))
{
	return 0;
}

VOID __stdargs __UserLibCleanup(REG(a4, APTR dataseg))
{
}

extern LONG safefail(VOID);

static int failures;

static void check(int ok, const char *what)
{
	if (!ok) {
		printf("FAIL: %s\n", what);
		failures++;
	}
}

int main(void)
{
	ULONG a;
	const struct Resident *romtag = NULL;

	check(__FuncTable__[1] == (APTR)LibOpen, "vector 1 is LibOpen");
	check(__FuncTable__[2] == (APTR)LibClose, "vector 2 is LibClose");
	check(__FuncTable__[3] == (APTR)LibExpunge, "vector 3 is LibExpunge");
	check(__FuncTable__[4] == (APTR)LibExtFunc, "vector 4 is LibExtFunc");

	/* The romtag is static and unreferenced: look for it between the
	   startup code and the function table, which the linker puts after
	   the code and read-only data.  Addresses as integers: pointers into
	   different objects don't compare. */
	for (a = (ULONG)safefail; a < (ULONG)__FuncTable__; a += 2)
		if (*(const UWORD *)a == RTC_MATCHWORD && (ULONG)((const struct Resident *)a)->rt_MatchTag == a)
			romtag = (const struct Resident *)a;
	check(romtag != NULL, "the romtag is there");
	if (romtag)
		check(romtag->rt_Name == (char *)LibName, "the romtag names the library");

	return failures != 0;
}
