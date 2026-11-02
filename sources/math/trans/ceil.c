#ifdef __HAVE_68881__

#include "fpu_math.h"

double ceil(double x)
{ return __fpu_round_to_integer(x, 0); }

#else

#include <proto/mathieeedoubbas.h>
#include "math_errno.h"

double ceil(double x)
{
  if (__math_special_double(x) || __math_zero_double(x))
    return x;
  return IEEEDPCeil(x);
}

#endif
