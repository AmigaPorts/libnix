#include "math_errno.h"

float __math_sqrt_specialf(float x)
{
  if (__math_zero_float(x) || (__math_inf_float(x) && !__math_negative(x)))
    return x;
  return __math_domainf(x);
}
