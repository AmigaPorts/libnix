#include <proto/dos.h>
#include <unistd.h>
#include "amigapath.h"

asm("_link: .global _link");
int symlink(const char *from, const char *to) {
	/* the link name is resolved by dos.library now, the target when the
	   link is followed; both get the Unix path syntax rewritten */
#ifdef IXPATHS
	if((from=__amigapath(from))==NULL)
		return -1;
	if((to=__amigapath(to))==NULL)
		return -1;
#endif
	return DOSTRUE != MakeLink((STRPTR )to, (LONG )from, LINK_SOFT);
}
