#include <math.h>
#include "ldbits.h"

/* Exact unless the result overflows or underflows, when the one
   multiplication rounds it: x * 2^n, with n clamped in steps so that the
   power of 2 itself stays representable.  */
long double scalbnl(long double x, int n)
{
  union ldbits u;
  if (n > 16383) {
    x *= 0x1p16383L;
    n -= 16383;
    if (n > 16383) {
      x *= 0x1p16383L;
      n -= 16383;
      if (n > 16383)
        n = 16383;
    }
  } else if (n < -16382) {
    /* keep 64 bits of headroom, so a normal x does not round here */
    x *= 0x1p-16382L * 0x1p64L;
    n += 16382 - 64;
    if (n < -16382) {
      x *= 0x1p-16382L * 0x1p64L;
      n += 16382 - 64;
      if (n < -16382)
        n = -16382;
    }
  }
  u.p.se = 0x3fff + n;
  u.p.pad = 0;
  u.p.hi = 0x80000000UL;
  u.p.lo = 0;
  return x * u.v;
}
