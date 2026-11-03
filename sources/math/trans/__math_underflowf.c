#include "math_errno.h"

float __math_underflowf(float sign)
{
  return __math_rangef(__math_negative(sign) ? -0.0f : 0.0f);
}
