#include <math.h>
#include "ldbits.h"
#include "math_errno.h"

/* Exact unless the result overflows or underflows, when the one
   multiplication rounds it: x * 2^n, with n clamped in steps so that the
   power of 2 itself stays representable.  */
long double scalbnl(long double x, int n)
{
  union ldbits u;
#ifdef __MATH_SETS_ERRNO
  union ldbits r;
  int exact;
  u.v = x;
  exact = LDBL_EXPFIELD(u) == 0x7fff || !(u.p.hi | u.p.lo);
#endif
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
#ifdef __MATH_SETS_ERRNO
  r.v = x * u.v;
  if (!exact && (LDBL_EXPFIELD(r) == 0x7fff || !(r.p.hi | r.p.lo)))
    errno = ERANGE;
  return r.v;
#else
  return x * u.v;
#endif
}
