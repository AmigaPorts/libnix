#include <math.h>
#include "ldbits.h"

/* Exact: x's magnitude with y's sign bit (also right for zeros and NaNs).  */
long double copysignl(long double x, long double y)
{
  return __ldbl_copysign(x, y);
}
