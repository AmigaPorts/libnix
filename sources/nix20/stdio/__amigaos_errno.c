/* libstdc++ is built with newlib's headers, and for gcc's amigaos thread
   model they read errno through __amigaos_errno(), as libnix's __errno
   is a pointer and not a function.  The same errno, for libnix programs
   that link libstdc++.  Not in __sf.c, whose top-level asm leaves the
   assembler in .bss.  */
#include <errno.h>

__stdargs int *__amigaos_errno(void) {
	return __errno;
}
