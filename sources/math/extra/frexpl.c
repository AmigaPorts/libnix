#include <math.h>

/* frexpl for the m68k 96-bit extended format: a sign and 15-bit biased
   exponent, 16 unused bits, and a 64-bit mantissa with an explicit
   integer bit.  The value is mantissa * 2^(exponent - 16383 - 63) for
   every exponent field, 0 included (x87 instead scales 0 like 1), so a
   mantissa without its integer bit (a denormal, or an unnormal) is just
   shifted up until the bit is set.  */
union ldbits {
  long double v;
  struct {
    unsigned short se;
    unsigned short pad;
    unsigned long hi;
    unsigned long lo;
  } p;
};

long double frexpl(long double x, int *e)
{
  union ldbits u;
  int exp;

  u.v = x;
  exp = u.p.se & 0x7fff;
  *e = 0;
  if (exp == 0x7fff)                  /* infinity or NaN */
    return x;
  if (!(u.p.hi | u.p.lo))             /* zero, whatever the exponent */
    return x;
  while (!(u.p.hi & 0x80000000UL)) {
    u.p.hi = (u.p.hi << 1) | (u.p.lo >> 31);
    u.p.lo <<= 1;
    exp--;
  }
  *e = exp - 0x3ffe;
  u.p.se = (u.p.se & 0x8000) | 0x3ffe;
  return u.v;
}
