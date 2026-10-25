#include <math.h>
#include "ldbits.h"

/* Exact: a NaN argument loses to the other one.  */
long double fminl(long double x, long double y)
{
  if (__ldbl_isnan(x))
    return y;
  if (__ldbl_isnan(y))
    return x;
  return x < y ? x : y;
}
