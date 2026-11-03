#include "math_errno.h"

/* For fmod(), remainder() and remquo(), with x or y special or y zero.  */
double __math_fmod_special(double x, double y)
{
  if (__math_nan_double(x))
    return x;
  if (__math_nan_double(y))
    return y;
  if (__math_inf_double(x) || __math_zero_double(y))
    return __math_domain(x);
  return x;
}
