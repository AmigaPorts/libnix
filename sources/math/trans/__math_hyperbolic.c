#include <proto/mathieeedoubtrans.h>
#include "math_errno.h"

/* sinh() and cosh() of |x| >= 512, whose result needs more than the library
   has: it stops at the end of exp(), which is before theirs, and sinh(1000) is
   8.5e307.  Half the exponent, twice, gets all the way.  */
double __math_hyperbolic(double x, double sign)
{
  double t;

  if (__math_nan_double(x))
    return x;
  *(__math_long *)&x &= 0x7fffffffUL;
  if (__math_inf_double(x))
    return __math_negative(sign) ? -x : x;
  if (x > 710.4758600739439)
    return __math_overflow(sign);
  t = IEEEDPExp(0.5 * x);
  t = (0.5 * t) * t;
  return __math_negative(sign) ? -t : t;
}
