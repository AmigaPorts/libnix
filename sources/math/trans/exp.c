#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, exp, "fetox")

#else

#include <proto/mathieeedoubtrans.h>
#include "math_errno.h"

double exp(double x)
{
  double result;

  /* From log(0x1p-1075) down, the result rounds to 0.  */
  if (__math_power_double(x) >= 9 &&
      (__math_special_double(x) || x > 709.78271289338397 ||
       x <= -745.13321910194122))
    return __math_exp_special(x);
  /* The library returns 0 for any result below DBL_MIN, from about -708.4
     down, so a 0 is an underflow too.  */
  result = IEEEDPExp(x);
  return result == 0 ? __math_underflow(1.0) : result;
}

#endif
