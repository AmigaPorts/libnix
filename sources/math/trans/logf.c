#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, logf, "flogn")

#else

#include <proto/mathieeesingtrans.h>
#include "math_errno.h"

float logf(float x)
{
  if (__math_not_positive_float(x))
    return __math_log_specialf(x);
  return IEEESPLog(x);
}

#endif
