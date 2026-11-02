#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, asin, "fasin")

#else

#include <proto/mathieeedoubtrans.h>
#include "math_errno.h"

double asin(double x)
{
  if (__math_above_one_double(x))
    return __math_domain(x);
  return IEEEDPAsin(x);
}

#endif
