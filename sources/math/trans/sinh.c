#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, sinh, "fsinh")

#else

#include <proto/mathieeedoubtrans.h>
#include "math_errno.h"

double sinh(double x)
{
  if (__math_power_double(x) >= 9)
    return __math_hyperbolic(x, x);
  return IEEEDPSinh(x);
}

#endif
