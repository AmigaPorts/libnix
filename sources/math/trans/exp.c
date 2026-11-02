#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, exp, "fetox")

#else

#include <proto/mathieeedoubtrans.h>
#include "math_errno.h"

double exp(double x)
{
  if (__math_power_double(x) >= 9 &&
      (__math_special_double(x) || x > 709.78271289338397 ||
       x < -745.13321910194122))
    return __math_exp_special(x);
  return IEEEDPExp(x);
}

#endif
