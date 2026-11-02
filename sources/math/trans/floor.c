#ifdef __HAVE_68881__

#include "fpu_math.h"

double floor(double x)
{ return __fpu_round_to_integer(x, SIGN_BIT); }

#else

#include <proto/mathieeedoubbas.h>
#include "math_errno.h"

double floor(double x)
{
  if (__math_special_double(x) || __math_zero_double(x))
    return x;
  return IEEEDPFloor(x);
}

#endif
