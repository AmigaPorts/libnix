#include "math_errno.h"

double __math_underflow(double sign)
{
  return __math_range(__math_negative(sign) ? -0.0 : 0.0);
}
