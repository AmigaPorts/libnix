#include <proto/mathieeesingtrans.h>
#include <float.h>
#include <math.h>
#include "math_errno.h"

/* The same as pow(): IEEESPPow() needs a positive finite x and a finite y.  */
float powf(float x, float y)
{
  float result;
  int integer, negative = __math_negative(x);

  if (__math_zero_float(y) ||
      (__math_one_float(x) && !negative))
    return 1.0f;
  if (__math_nan_float(x))
    return x;
  if (__math_nan_float(y))
    return y;

  integer = __math_integerf(y);

  if (__math_inf_float(y)) {
    if (__math_one_float(x))
      return 1.0f;
    return __math_above_one_float(x) != __math_negative(y) ?
	   __builtin_huge_valf() : 0.0f;
  }
  if (__math_zero_float(x)) {
    if (__math_negative(y)) {
      errno = ERANGE;
      return negative && integer == 2 ? -__builtin_huge_valf() : __builtin_huge_valf();
    }
    return integer == 2 ? x : 0.0f;
  }
  if (__math_inf_float(x)) {
    if (__math_negative(y))
      return negative && integer == 2 ? -0.0f : 0.0f;
    return negative && integer == 2 ? x : __builtin_huge_valf();
  }
  if (negative) {
    if (integer == 0) {
      errno = EDOM;
      return __builtin_nanf("");
    }
    x = -x;
  }

  /* From |y| = 1e10 on, every x but 1 is out of range, and IEEESPPow()
     returns 1 for powf(10,FLT_MAX).  Such a y is even, so x = -1 is 1 too.  */
  if (fabsf(y) >= 1e10f && !__math_one_float(x))
    return __math_above_one_float(x) != __math_negative(y) ?
	   __math_overflowf(1.0f) : __math_underflowf(1.0f);

  result = IEEESPPow(y, x);

  /* An overflow comes back as infinity, which y * logf(x) can't always
     tell from FLT_MAX: powf(2,128) is 88.72284f either way.  */
  if (__math_suspect_float(result)) {
    if (__math_inf_float(result) || y * logf(x) > 88.7229f)
      result = __math_overflowf(1.0f);
  } else if (__math_zero_float(result)) {
    errno = ERANGE;
  }

  return negative && integer == 2 ? -result : result;
}
