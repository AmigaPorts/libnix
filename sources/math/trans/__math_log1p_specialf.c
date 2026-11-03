#include "math_errno.h"

float __math_log1p_specialf(float x)
{
  if (__math_inf_float(x) && !__math_negative(x))
    return x;
  if (__math_one_float(x) && __math_negative(x))
    return __math_rangef(-__builtin_huge_valf());
  return __math_domainf(x);
}
