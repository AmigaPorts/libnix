#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, tanh, "ftanh")

#else

#include <proto/mathieeedoubtrans.h>
#include "math_errno.h"

double tanh(double x)
{
  if (__math_nan_double(x))
    return x;
  return IEEEDPTanh(x);
}

#endif
