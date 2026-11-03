#include "math_errno.h"

/* With x, y or z NaN or infinite.  */
double __math_fma_special(double x, double y, double z)
{
  int negative;

  if (__math_nan_double(x))
    return x;
  if (__math_nan_double(y))
    return y;
  if (__math_nan_double(z))
    return z;
  if (!__math_inf_double(x) && !__math_inf_double(y))
    return z;
  if (__math_zero_double(x) || __math_zero_double(y))
    return __math_domain(x);
  negative = __math_negative(x) != __math_negative(y);
  if (__math_inf_double(z) && negative != __math_negative(z))
    return __math_domain(x);
  return negative ? -__builtin_huge_val() : __builtin_huge_val();
}
