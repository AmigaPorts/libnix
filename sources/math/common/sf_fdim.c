/* Copyright (C) 2002 by  Red Hat, Incorporated. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

#include "fdlibm.h"
#include "math_errno.h"
#include <float.h>

#ifdef __STDC__
	float fdimf(float x, float y)
#else
	float fdimf(x,y)
	float x;
	float y;
#endif
{
  int c = __fpclassifyf(x);
  if (c == FP_NAN)  return(x);
  if (__fpclassifyf(y) == FP_NAN)  return(y);
  if (c == FP_INFINITE)
    return !__math_negative(x) && !(__math_inf_float(y) && !__math_negative(y)) ? HUGE_VALF : 0.0;
  if (__math_inf_float(y))
    return __math_negative(y) ? HUGE_VALF : 0.0;
#ifdef __MATH_SETS_ERRNO
  if (x > y) {
    float result = x - y;
    if (__math_suspect_float(result) && 0.5f * x - 0.5f * y > FLT_MAX / 2)
      return __math_overflowf(1.0f);
    return result;
  }
  return 0.0;
#else
  return x > y ? x - y : 0.0;
#endif
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double fdim(double x, double y)
#else
	double fdim(x,y)
	double x;
	double y;
#endif
{
  return (double) fdimf((float) x, (float) y);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
