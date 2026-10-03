#include <proto/dos.h>
#include <unistd.h>

asm(".global _link; _link = _symlink");
int symlink(const char *from, const char *to) {
	return DOSTRUE != MakeLink((STRPTR )to, (LONG )from, LINK_SOFT);
}
