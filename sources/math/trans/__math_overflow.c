#include "math_errno.h"

double __math_overflow(double sign)
{
  return __math_range(__math_negative(sign) ? -__builtin_huge_val() : __builtin_huge_val());
}
