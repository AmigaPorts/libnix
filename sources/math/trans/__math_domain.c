#include "math_errno.h"

/* A NaN goes through, and anything else is a domain error.  */
double __math_domain(double x)
{
  if (__math_nan_double(x))
    return x;
  errno = EDOM;
  return __builtin_nan("");
}
