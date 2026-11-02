#include <math.h>
#include <float.h>
#include "math_errno.h"

#define SCALE_UP   0x1p600
#define SCALE_DOWN 0x1p-600

/* a*a overflows halfway to where the result does, and mathieeedoubbas.library
   stops at DBL_MAX rather than saying so.  */
double hypot(double a, double b)
{
  double result;
  int power_a, power_b;

  if (__math_inf_double(a) || __math_inf_double(b))
    return __builtin_huge_val();
  if (__math_nan_double(a))
    return a;
  if (__math_nan_double(b))
    return b;

  *(__math_long *)&a &= 0x7fffffffUL;
  *(__math_long *)&b &= 0x7fffffffUL;

  if (__math_zero_double(a))
    return b;
  if (__math_zero_double(b))
    return a;

  power_a = __math_power_double(a);
  power_b = __math_power_double(b);

  if (power_a - power_b > 60)
    return a;
  if (power_b - power_a > 60)
    return b;

  if (power_a > 450 || power_b > 450) {
    a *= SCALE_DOWN;
    b *= SCALE_DOWN;
    result = sqrt(a*a + b*b);
    if (result > DBL_MAX * SCALE_DOWN)
      return __MATH_SOFT(result * SCALE_UP, __math_overflow(1.0));
    return result * SCALE_UP;
  }
  if (power_a < -450 || power_b < -450) {
    a *= SCALE_UP;
    b *= SCALE_UP;
    return sqrt(a*a + b*b) * SCALE_DOWN;
  }

  return sqrt(a*a + b*b);
}
