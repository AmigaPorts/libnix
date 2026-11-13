#if 1

#include <exec/types.h>

ULONG FastRand(ULONG seed)
{ ULONG a=seed<<1;
  if((LONG)seed<=0)
    a^=0x1d872b41;
  return a;
}

#else

asm(
"		.text\n"

"		.globl	_FastRand\n"

"_FastRand:	movel	sp@(4:W),d0\n"
"		addl	d0,d0\n"
"		bhis	L1\n"
"		eoril	#0x1D872B41,d0\n"
"L1:		rts\n"
);

#endif
