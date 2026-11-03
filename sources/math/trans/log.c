#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, log, "flogn")

#else

#include <proto/mathieeedoubtrans.h>
#include "math_errno.h"

double log(double x)
{
  if (__math_not_positive_double(x))
    return __math_log_special(x);
  return IEEEDPLog(x);
}

#endif
