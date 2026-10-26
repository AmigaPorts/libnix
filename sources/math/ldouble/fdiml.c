#include <math.h>
#include "ldbits.h"

/* The positive difference, or +0; a NaN argument gives a NaN.  */
long double fdiml(long double x, long double y)
{
  if (__ldbl_isnan(x) || __ldbl_isnan(y))
    return x + y;
  return x > y ? x - y : 0.0L;
}
