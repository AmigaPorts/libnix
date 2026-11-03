#include "math_errno.h"

float __math_fmod_specialf(float x, float y)
{
  if (__math_nan_float(x))
    return x;
  if (__math_nan_float(y))
    return y;
  if (__math_inf_float(x) || __math_zero_float(y))
    return __math_domainf(x);
  return x;
}
