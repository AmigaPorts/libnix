#include "math_errno.h"

float __math_overflowf(float sign)
{
  return __math_rangef(__math_negative(sign) ? -__builtin_huge_valf() : __builtin_huge_valf());
}
