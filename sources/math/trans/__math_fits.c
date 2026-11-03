#include "math_errno.h"

/* Is rounded inside a type of bits value bits and a sign?  -2^bits is.  */
int __math_fits(double rounded, int bits)
{
  int power = __math_power_double(rounded);

  if (power != bits)
    return power < bits;
  return __math_negative(rounded) &&
	 ((__math_high(rounded) & 0xfffffUL) | __math_low(rounded)) == 0;
}
