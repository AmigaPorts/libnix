#include <math.h>
#include "ldbits.h"

/* A quiet NaN, built on the bits: -ffast-math may fold any arithmetic
   that makes one.  The payload string is ignored, as nan() does.  */
long double nanl(const char *s)
{
  union ldbits u;
  (void)s;
  u.p.se = 0x7fff;
  u.p.pad = 0;
  u.p.hi = 0xc0000000UL;
  u.p.lo = 0;
  return u.v;
}
