#include "math_errno.h"

float __math_domainf(float x)
{
  if (__math_nan_float(x))
    return x;
  errno = EDOM;
  return __builtin_nanf("");
}
