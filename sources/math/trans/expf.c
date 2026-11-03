#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, expf, "fetox")

#else

#include <proto/mathieeesingtrans.h>
#include "math_errno.h"

float expf(float x)
{
  float result;

  if (__math_power_float(x) >= 6 &&
      (__math_special_float(x) || x > 88.72283f || x < -103.97208f))
    return __math_exp_specialf(x);
  /* The library returns 0 for any result below FLT_MIN, from about -87.3
     down, so a 0 is an underflow too.  */
  result = IEEESPExp(x);
  return result == 0 ? __math_underflowf(1.0f) : result;
}

#endif
