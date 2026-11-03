#include "math_errno.h"

/* 0 for a number with a fraction, 1 for an even integer, 2 for an odd one.  */
int __math_integer(double x)
{
  int power = __math_power_double(x), shift;
  unsigned long high, low = __math_low(x);

  if (power < 0)
    return 0;
  if (power >= 53)
    return 1;
  high = (__math_high(x) & 0xfffffUL) | 0x100000UL;
  if (power <= 20) {
    shift = 20 - power;
    if (low != 0 || (high & ((1UL << shift) - 1)) != 0)
      return 0;
    return (high >> shift) & 1 ? 2 : 1;
  }
  if (power == 52)
    return low & 1 ? 2 : 1;
  shift = 52 - power;
  if ((low & ((1UL << shift) - 1)) != 0)
    return 0;
  return (low >> shift) & 1 ? 2 : 1;
}
