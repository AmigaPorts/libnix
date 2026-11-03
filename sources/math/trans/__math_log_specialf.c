#include "math_errno.h"

float __math_log_specialf(float x)
{
  if (__math_zero_float(x))
    return __math_rangef(-__builtin_huge_valf());
  if (__math_inf_float(x) && !__math_negative(x))
    return x;
  return __math_domainf(x);
}
