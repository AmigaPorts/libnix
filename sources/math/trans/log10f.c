#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, log10f, "flog10")

#else

#include <proto/mathieeesingtrans.h>
#include "math_errno.h"

float log10f(float x)
{
  if (__math_not_positive_float(x))
    return __math_log_specialf(x);
  return IEEESPLog10(x);
}

#endif
