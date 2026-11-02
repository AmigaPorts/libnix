#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, asinf, "fasin")

#else

#include <proto/mathieeesingtrans.h>
#include "math_errno.h"

float asinf(float x)
{
  if (__math_above_one_float(x))
    return __math_domainf(x);
  return IEEESPAsin(x);
}

#endif
