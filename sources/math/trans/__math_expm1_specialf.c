#include "math_errno.h"

float __math_expm1_specialf(float x)
{
  if (__math_nan_float(x))
    return x;
  if (__math_inf_float(x))
    return __math_negative(x) ? -1.0f : x;
  return __math_overflowf(1.0f);
}
