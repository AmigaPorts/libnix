#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, log10, "flog10")

#else

#include <proto/mathieeedoubtrans.h>
#include "math_errno.h"

double log10(double x)
{
  if (__math_not_positive_double(x))
    return __math_log_special(x);
  return IEEEDPLog10(x);
}

#endif
