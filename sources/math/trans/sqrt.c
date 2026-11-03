#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION_FPCR(double, sqrt, "fsqrt", FPCR_PRECISION, FPCR_DOUBLE)

#else

#include <proto/mathieeedoubtrans.h>
#include "math_errno.h"

double sqrt(double x)
{
  if (__math_not_positive_double(x))
    return __math_sqrt_special(x);
  return IEEEDPSqrt(x);
}

#endif
