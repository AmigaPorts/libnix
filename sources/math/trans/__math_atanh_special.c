#include "math_errno.h"

double __math_atanh_special(double x)
{
  if (__math_one_double(x))
    return __math_overflow(x);
  return __math_domain(x);
}
