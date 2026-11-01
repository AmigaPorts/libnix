#include <proto/mathieeedoubtrans.h>

/* mathieeedoubtrans.library takes an infinity for a large number, and a zero or
   a subnormal number for the smallest normal one.  */
double __extendsfdf2(float x)
{
  union { float f; unsigned long w; } in;
  union { double d; unsigned long w[2]; } out;
  unsigned long exponent;

  in.f = x;

  exponent = in.w & 0x7f800000;
  if (exponent != 0)
    {
      if (exponent != 0x7f800000)
	return IEEEDPFieee(x);
    }
  else
    {
      unsigned long mantissa = in.w & 0x007fffff;

      out.w[0] = in.w & 0x80000000;
      out.w[1] = 0;

      if (mantissa != 0)
	{
	  exponent = 1023 - 126;

	  while ((mantissa & 0x00800000) == 0)
	    {
	      mantissa <<= 1;
	      exponent--;
	    }

	  mantissa &= 0x007fffff;

	  out.w[0] |= (exponent << 20) | (mantissa >> 3);
	  out.w[1] = mantissa << 29;
	}

      return out.d;
    }

  out.w[0] = (in.w & 0x80000000) | 0x7ff00000;
  out.w[1] = 0;

  if ((in.w & 0x007fffff) != 0)
    out.w[0] |= 0x00080000;

  return out.d;
}
