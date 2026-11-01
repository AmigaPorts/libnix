#include <proto/mathieeedoubtrans.h>

/* mathieeedoubtrans.library never comes back from an infinity, so what doesn't
   fit doesn't go there.  It also loses the sign of zero.  */
float __truncdfsf2(double x)
{
  union { double d; unsigned long w[2]; } in;
  union { float f; unsigned long w; } out;
  unsigned long exponent;

  in.d = x;

  exponent = in.w[0] & 0x7ff00000;
  if (exponent < 0x47f00000)
    {
      if (((in.w[0] << 1) | in.w[1]) != 0)
	return IEEEDPTieee(x);

      out.w = in.w[0];

      return out.f;
    }

  out.w = (in.w[0] & 0x80000000) | 0x7f800000;

  if (exponent == 0x7ff00000 && ((in.w[0] & 0x000fffff) | in.w[1]) != 0)
    out.w |= 0x00400000;

  return out.f;
}
