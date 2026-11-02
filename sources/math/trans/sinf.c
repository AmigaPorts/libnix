#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, sinf, "fsin")

#else

#include <proto/mathieeesingtrans.h>
#include "math_errno.h"

float sinf(float x)
{
  if (__math_special_float(x))
    return __math_domainf(x);
  return IEEESPSin(x);
}

#endif
