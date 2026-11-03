#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, cosf, "fcos")

#else

#include <proto/mathieeesingtrans.h>
#include "math_errno.h"

float cosf(float x)
{
  if (__math_special_float(x))
    return __math_domainf(x);
  return IEEESPCos(x);
}

#endif
