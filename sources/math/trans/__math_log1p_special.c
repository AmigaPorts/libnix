#include "math_errno.h"

double __math_log1p_special(double x)
{
  if (__math_inf_double(x) && !__math_negative(x))
    return x;
  if (__math_one_double(x) && __math_negative(x))
    return __math_range(-__builtin_huge_val());
  return __math_domain(x);
}
