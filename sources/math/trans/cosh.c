#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, cosh, "fcosh")

#else

#include <proto/mathieeedoubtrans.h>
#include "math_errno.h"

double cosh(double x)
{
  if (__math_power_double(x) >= 9)
    return __math_hyperbolic(x, 1.0);
  return IEEEDPCosh(x);
}

#endif
