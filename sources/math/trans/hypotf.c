#include <math.h>
#include "math_errno.h"

/* In double, which has the room, and rounds a*a less.  */
float hypotf(float a, float b)
{
  double x = a, y = b;
  float result;

  if (__math_inf_float(a) || __math_inf_float(b))
    return __builtin_huge_valf();
  if (__math_nan_float(a))
    return a;
  if (__math_nan_float(b))
    return b;

  result = (float)sqrt(x*x + y*y);

  if (__math_special_float(result))
    errno = ERANGE;

  return result;
}
