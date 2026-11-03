#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, cos, "fcos")

#else

#include <proto/mathieeedoubtrans.h>
#include "math_errno.h"

double cos(double x)
{
  if (__math_special_double(x))
    return __math_domain(x);
  return IEEEDPCos(x);
}

#endif
