#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, acos, "facos")

#else

#include <proto/mathieeedoubtrans.h>
#include "math_errno.h"

double acos(double x)
{
  if (__math_above_one_double(x))
    return __math_domain(x);
  return IEEEDPAcos(x);
}

#endif
