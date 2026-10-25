#include <math.h>
#include "ldbits.h"

/* Exact: the unbiased exponent as a long double; -inf for 0, +inf for
   an infinity, and NaN for a NaN.  */
long double logbl(long double x)
{
  union ldbits u;
  u.v = x;
  if (LDBL_EXPFIELD(u) == 0x7fff)
    return __ldbl_isnan(x) ? x : __ldbl_inf(0);
  if (!(u.p.hi | u.p.lo))
    return __ldbl_inf(1);
  return ilogbl(x);
}
