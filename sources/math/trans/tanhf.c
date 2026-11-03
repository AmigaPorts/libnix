#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, tanhf, "ftanh")

#else

#include <proto/mathieeesingtrans.h>
#include "math_errno.h"

float tanhf(float x)
{
  if (__math_nan_float(x))
    return x;
  return IEEESPTanh(x);
}

#endif
