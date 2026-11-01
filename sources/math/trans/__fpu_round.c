#ifdef __HAVE_68881__

#include "fpu_math.h"

double __fpu_round_to_integer(double x, unsigned long away)
{
  union { double d; unsigned long w[2]; } u;
  unsigned long hi, lo, sign, mask, next;
  int exponent;

  u.d = x;
  hi = u.w[0];
  lo = u.w[1];
  sign = hi & SIGN_BIT;
  exponent = (int) (hi >> 20 & 0x7ff) - 1023;

  if (exponent == 1024)
    return x + x;
  if (exponent >= 52)
    return x;

  if (exponent < 0)
    {
      if (!(hi & ~SIGN_BIT) && !lo)
	return x;
      hi = sign == away ? sign | 0x3ff00000 : sign;
      lo = 0;
    }
  else if (exponent < 20)
    {
      mask = 0xfffff >> exponent;
      if (!(hi & mask) && !lo)
	return x;
      if (sign == away)
	hi += mask + 1;
      hi &= ~mask;
      lo = 0;
    }
  else
    {
      mask = 0xffffffffUL >> (exponent - 20);
      if (!(lo & mask))
	return x;
      if (sign == away)
	{
	  next = lo + mask;
	  if (next < lo)
	    hi++;
	  lo = next;
	}
      lo &= ~mask;
    }

  u.w[0] = hi;
  u.w[1] = lo;
  return u.d;
}

#endif
