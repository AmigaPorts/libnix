#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, sinhf, "fsinh")

#else

#include <proto/mathieeesingtrans.h>
#include "math_errno.h"

float sinhf(float x)
{
  if (__math_power_float(x) >= 6)
    return __math_hyperbolicf(x, x);
  return IEEESPSinh(x);
}

#endif
