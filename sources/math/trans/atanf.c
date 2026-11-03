#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, atanf, "fatan")

#else

#include <proto/mathieeesingtrans.h>
#include "math_errno.h"

float atanf(float x)
{
  if (__math_nan_float(x))
    return x;
  return IEEESPAtan(x);
}

#endif
