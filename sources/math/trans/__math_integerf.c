#include "math_errno.h"

int __math_integerf(float x)
{
  int power = __math_power_float(x), shift;
  unsigned long bits;

  if (power < 0)
    return 0;
  if (power >= 24)
    return 1;
  shift = 23 - power;
  bits = (__math_high(x) & 0x7fffffUL) | 0x800000UL;
  if ((bits & ((1UL << shift) - 1)) != 0)
    return 0;
  return (bits >> shift) & 1 ? 2 : 1;
}
