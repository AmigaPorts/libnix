/* The m68k 96-bit extended format: a sign and 15-bit biased exponent, 16
   unused bits, and a 64-bit mantissa with an explicit integer bit.  An
   exponent field of 0x7fff is an infinity if the fraction (the mantissa
   without its integer bit) is 0, else a NaN.

   libnix is built with -ffast-math, which lets the compiler assume no
   NaNs or infinities, so test for them here on the bits, never with
   isnan() or x != x.  */

union ldbits {
  long double v;
  struct {
    unsigned short se;
    unsigned short pad;
    unsigned long hi;
    unsigned long lo;
  } p;
};

#define LDBL_EXPFIELD(u) ((u).p.se & 0x7fff)

static inline int __ldbl_isnan(long double x)
{
  union ldbits u;
  u.v = x;
  return LDBL_EXPFIELD(u) == 0x7fff && ((u.p.hi & 0x7fffffffUL) | u.p.lo);
}

/* Is a rounded x inside a type of bits value bits and a sign?  -2^bits is.  */
static inline int __ldbl_fits(long double x, int bits)
{
  union ldbits u;
  int power;
  u.v = x;
  power = LDBL_EXPFIELD(u) - 0x3fff;
  if (power != bits)
    return power < bits;
  return (u.p.se & 0x8000) && u.p.hi == 0x80000000UL && u.p.lo == 0;
}

static inline long double __ldbl_inf(int negative)
{
  union ldbits u;
  u.p.se = negative ? 0xffff : 0x7fff;
  u.p.pad = 0;
  u.p.hi = 0;
  u.p.lo = 0;
  return u.v;
}

/* x with the sign of y.  */
static inline long double __ldbl_copysign(long double x, long double y)
{
  union ldbits ux, uy;
  ux.v = x;
  uy.v = y;
  ux.p.se = (ux.p.se & 0x7fff) | (uy.p.se & 0x8000);
  return ux.v;
}
