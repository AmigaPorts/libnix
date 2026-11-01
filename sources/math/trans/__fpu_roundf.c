#ifdef __HAVE_68881__

#include "fpu_math.h"

float __fpu_round_to_integerf(float x, unsigned long away)
{
  union { float f; unsigned long w; } u;
  unsigned long sign, mask;
  int exponent;

  u.f = x;
  sign = u.w & SIGN_BIT;
  exponent = (int) (u.w >> 23 & 0xff) - 127;

  if (exponent == 128)
    return x + x;
  if (exponent >= 23)
    return x;

  if (exponent < 0)
    {
      if (!(u.w & ~SIGN_BIT))
	return x;
      u.w = sign == away ? sign | 0x3f800000 : sign;
    }
  else
    {
      mask = 0x7fffff >> exponent;
      if (!(u.w & mask))
	return x;
      if (sign == away)
	u.w += mask + 1;
      u.w &= ~mask;
    }

  return u.f;
}

#endif
