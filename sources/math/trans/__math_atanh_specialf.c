#include "math_errno.h"

float __math_atanh_specialf(float x)
{
  if (__math_one_float(x))
    return __math_overflowf(x);
  return __math_domainf(x);
}
