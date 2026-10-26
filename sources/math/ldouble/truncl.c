#include <math.h>
#include "ldbits.h"

/* Exact: clear the mantissa bits below the binary point.  An |x| of 2^63
   or more is already integral, or is an infinity or a NaN; below 1 the
   result is a zero with x's sign.  */
long double truncl(long double x)
{
  union ldbits u;
  int e;
  u.v = x;
  e = LDBL_EXPFIELD(u) - 0x3fff;
  if (e >= 63)
    return x;
  if (e < 0) {
    u.p.se &= 0x8000;
    u.p.hi = 0;
    u.p.lo = 0;
    return u.v;
  }
  /* e + 1 integer bits, from the top of the mantissa */
  if (e < 31) {
    u.p.hi &= ~(0xffffffffUL >> (e + 1));
    u.p.lo = 0;
  } else if (e == 31)
    u.p.lo = 0;
  else
    u.p.lo &= ~(0xffffffffUL >> (e - 31));
  return u.v;
}
