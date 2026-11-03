#ifdef __HAVE_68881__

#include "fpu_math.h"

float ceilf(float x)
{ return __fpu_round_to_integerf(x, 0); }

#else

#include <proto/mathieeesingbas.h>
#include "math_errno.h"

float ceilf(float x)
{
  if (__math_special_float(x) || __math_zero_float(x))
    return x;
  return IEEESPCeil(x);
}

#endif
