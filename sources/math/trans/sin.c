#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, sin, "fsin")

#else

#include <proto/mathieeedoubtrans.h>
#include "math_errno.h"

double sin(double x)
{
  if (__math_special_double(x))
    return __math_domain(x);
  return IEEEDPSin(x);
}

#endif
