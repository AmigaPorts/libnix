#include "math_errno.h"

int __math_fitsf(float rounded, int bits)
{
  int power = __math_power_float(rounded);

  if (power != bits)
    return power < bits;
  return __math_negative(rounded) && (__math_high(rounded) & 0x7fffffUL) == 0;
}
