#include <proto/mathieeesingtrans.h>
#include "math_errno.h"

/* __math_hyperbolic(), for |x| >= 64: cosh(100) is 1.7e38.  */
float __math_hyperbolicf(float x, float sign)
{
  float t;

  if (__math_nan_float(x))
    return x;
  *(__math_long *)&x &= 0x7fffffffUL;
  if (__math_inf_float(x))
    return __math_negative(sign) ? -x : x;
  if (x > 89.41598f)
    return __math_overflowf(sign);
  t = IEEESPExp(0.5f * x);
  t = (0.5f * t) * t;
  return __math_negative(sign) ? -t : t;
}
