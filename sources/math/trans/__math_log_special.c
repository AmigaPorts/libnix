#include "math_errno.h"

double __math_log_special(double x)
{
  if (__math_zero_double(x))
    return __math_range(-__builtin_huge_val());
  if (__math_inf_double(x) && !__math_negative(x))
    return x;
  return __math_domain(x);
}
