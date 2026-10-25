#include <math.h>
#include "ldbits.h"

/* Exact: the next long double after x towards y, one step of the 64-bit
   mantissa.  An exponent field of 0 scales the mantissa like a field of
   1, so the denormals continue the normals with no special case: a
   carry out of the mantissa just moves to the next exponent.  */
long double nextafterl(long double x, long double y)
{
  union ldbits u;
  if (__ldbl_isnan(x) || __ldbl_isnan(y))
    return x + y;
  if (x == y)
    return y;
  u.v = x;
  if (LDBL_EXPFIELD(u) == 0x7fff) {
    /* from an infinity, which has a zero mantissa: +-LDBL_MAX */
    u.p.se--;
    u.p.hi = 0xffffffffUL;
    u.p.lo = 0xffffffffUL;
    return u.v;
  }
  if (!(u.p.hi | u.p.lo)) {
    /* the smallest denormal, with the sign of the direction */
    u.p.se = y < x ? 0x8000 : 0;
    u.p.lo = 1;
    return u.v;
  }
  if ((x < y) == !(u.p.se & 0x8000)) {
    /* away from zero */
    if (++u.p.lo == 0 && ++u.p.hi == 0) {
      u.p.hi = 0x80000000UL;
      u.p.se++;
      if (LDBL_EXPFIELD(u) == 0x7fff)
        u.p.hi = 0;                   /* overflow to infinity */
    }
  } else if (u.p.hi == 0x80000000UL && u.p.lo == 0 && LDBL_EXPFIELD(u) > 0) {
    /* towards zero from a power of 2: all ones, one exponent lower */
    u.p.se--;
    u.p.hi = 0xffffffffUL;
    u.p.lo = 0xffffffffUL;
  } else {
    /* towards zero */
    if (u.p.lo-- == 0)
      u.p.hi--;
  }
  return u.v;
}
