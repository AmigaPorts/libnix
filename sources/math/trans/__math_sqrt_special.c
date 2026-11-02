#include "math_errno.h"

double __math_sqrt_special(double x)
{
  if (__math_zero_double(x) || (__math_inf_double(x) && !__math_negative(x)))
    return x;
  return __math_domain(x);
}
