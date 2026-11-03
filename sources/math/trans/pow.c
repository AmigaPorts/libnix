#include <proto/mathieeedoubtrans.h>
#include <float.h>
#include <math.h>
#include "math_errno.h"

/* IEEEDPPow() is only for a positive finite x and a finite y: pow(0,-1) is 1,
   pow(-1,0.5) is 0, and pow(2,2000) stops at DBL_MAX.  */
double pow(double x, double y)
{
  double result;
  int integer, negative = __math_negative(x);

  if (__math_zero_double(y) ||
      (__math_one_double(x) && !negative))
    return 1.0;
  if (__math_nan_double(x))
    return x;
  if (__math_nan_double(y))
    return y;

  integer = __math_integer(y);

  if (__math_inf_double(y)) {
    if (__math_one_double(x))
      return 1.0;
    return __math_above_one_double(x) != __math_negative(y) ?
	   __builtin_huge_val() : 0.0;
  }
  if (__math_zero_double(x)) {
    if (__math_negative(y)) {
      errno = ERANGE;
      return negative && integer == 2 ? -__builtin_huge_val() : __builtin_huge_val();
    }
    return integer == 2 ? x : 0.0;
  }
  if (__math_inf_double(x)) {
    if (__math_negative(y))
      return negative && integer == 2 ? -0.0 : 0.0;
    return negative && integer == 2 ? x : __builtin_huge_val();
  }
  if (negative) {
    if (integer == 0) {
      errno = EDOM;
      return __builtin_nan("");
    }
    x = -x;
  }

  /* From |y| = 1e19 on, every x but 1 is out of range, and IEEEDPPow()
     returns 1 for pow(10,DBL_MAX).  Such a y is even, so x = -1 is 1 too.  */
  if (fabs(y) >= 1e19 && !__math_one_double(x))
    return __math_above_one_double(x) != __math_negative(y) ?
	   __math_overflow(1.0) : __math_underflow(1.0);

  result = IEEEDPPow(y, x);

  /* An overflow comes back as DBL_MAX or infinity, but also as less than
     DBL_MAX further out: pow(1e300,2.5) is 1.36e308.  For DBL_MAX itself,
     the limit is just below log(DBL_MAX), as 1024*log(2) can't be told
     from it.  */
  if (__math_suspect_double(result)) {
    if (__math_inf_double(result) ||
	(y != 1.0 &&
	 y * log(x) > (result == DBL_MAX ? 709.7827128933 : 709.79)))
      result = __math_overflow(1.0);
  } else if (__math_zero_double(result)) {
    errno = ERANGE;
  }

  return negative && integer == 2 ? -result : result;
}
