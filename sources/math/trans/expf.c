#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, expf, "fetox")

#else

#include <proto/mathieeesingtrans.h>
#include "math_errno.h"

float expf(float x)
{
  if (__math_power_float(x) >= 6 &&
      (__math_special_float(x) || x > 88.72283f || x < -103.97208f))
    return __math_exp_specialf(x);
  return IEEESPExp(x);
}

#endif
