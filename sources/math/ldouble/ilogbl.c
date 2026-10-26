#include <limits.h>
#include <math.h>
#include "ldbits.h"

/* Exact: the unbiased exponent, through frexpl, which normalizes
   denormals.  */
int ilogbl(long double x)
{
  union ldbits u;
  int e;
  u.v = x;
  if (LDBL_EXPFIELD(u) == 0x7fff)
    return __ldbl_isnan(x) ? FP_ILOGBNAN : INT_MAX;
  if (!(u.p.hi | u.p.lo))
    return FP_ILOGB0;
  frexpl(x, &e);
  return e - 1;
}
