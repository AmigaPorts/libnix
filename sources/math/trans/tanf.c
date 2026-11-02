#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, tanf, "ftan")

#else

#include <proto/mathieeesingtrans.h>
#include "math_errno.h"

float tanf(float x)
{
  if (__math_special_float(x))
    return __math_domainf(x);
  return IEEESPTan(x);
}

#endif
