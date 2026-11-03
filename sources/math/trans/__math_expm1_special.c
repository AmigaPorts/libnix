#include "math_errno.h"

double __math_expm1_special(double x)
{
  if (__math_nan_double(x))
    return x;
  if (__math_inf_double(x))
    return __math_negative(x) ? -1.0 : x;
  return __math_overflow(1.0);
}
