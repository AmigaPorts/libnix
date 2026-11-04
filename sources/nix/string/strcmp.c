#define __NO_INLINE__
#include <string.h>

/* GCC 16 hoists the second byte load across the zero check, requiring
   another tst.b, and sinks the first pointer increment, preventing a
   post-increment load.  Keep the matching-byte loop at five instructions
   on m68k without changing optimization of the other string functions. */
#if defined(__m68k__) && __GNUC__ >= 16
__attribute__((optimize("no-code-hoisting", "no-tree-sink")))
#endif
int strcmp(const char *p1,const char *p2)
{ 
	unsigned char a,b;
  for(;;) {
	  char c = *p1++;
	  if (c) {
		if (c == *p2++)
		  continue;
	  } else
		++p2;

	  a = *--p1;
	  b = *--p2;
	  break;
	}
  return (short)a - b;
}
