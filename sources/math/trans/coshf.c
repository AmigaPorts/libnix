#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, coshf, "fcosh")

#else

#include <proto/mathieeesingtrans.h>
#include "math_errno.h"

float coshf(float x)
{
  if (__math_power_float(x) >= 6)
    return __math_hyperbolicf(x, 1.0f);
  return IEEESPCosh(x);
}

#endif
