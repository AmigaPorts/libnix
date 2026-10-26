#include <math.h>
#include "ldbits.h"

/* Exact: round to an integer in the current rounding mode.  Below 2^63,
   adding and subtracting 2^63 leaves no bits below the binary point, and
   the addition rounds as the mode says.  volatile keeps -ffast-math from
   cancelling the two out; the sign of a zero result comes from x.  */
long double rintl(long double x)
{
  union ldbits u;
  volatile long double t;
  u.v = x;
  if (LDBL_EXPFIELD(u) >= 0x3fff + 63)
    return x;
  if (u.p.se & 0x8000) {
    t = x - 0x1p63L;
    t = t + 0x1p63L;
  } else {
    t = x + 0x1p63L;
    t = t - 0x1p63L;
  }
  return __ldbl_copysign(t, x);
}
