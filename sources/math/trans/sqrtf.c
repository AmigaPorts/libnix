#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION_FPCR(float, sqrtf, "fsqrt", FPCR_PRECISION, FPCR_SINGLE)

#else

#include <proto/mathieeesingtrans.h>
#include "math_errno.h"

float sqrtf(float x)
{
  if (__math_not_positive_float(x))
    return __math_sqrt_specialf(x);
  return IEEESPSqrt(x);
}

#endif
