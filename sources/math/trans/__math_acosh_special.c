#include "math_errno.h"

double __math_acosh_special(double x)
{
  if (__math_inf_double(x) && !__math_negative(x))
    return x;
  return __math_domain(x);
}
