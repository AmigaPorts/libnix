/* With -m68881, the math functions use the FPU itself.  mathieeedoubtrans.library
   would only pass the result back, and it loses the exception flags and the
   rounding mode, which fenv.h is about.  */

#ifndef _FPU_MATH_H
#define _FPU_MATH_H

/* Stored in a volatile, the result is a double or a float with the flags of one,
   not the extended value left in fp0.  */
#define FPU_FUNCTION(type, name, insn)					\
  type name(type x)							\
  {									\
    volatile type result;						\
    __asm__ (insn "%.x %1,%0" : "=f" (result) : "f" (x));		\
    return result;							\
  }

/* The FPCR for a precision for the time of one instruction: fsqrt in the
   precision of its result rounds only once.  */
#define FPU_FUNCTION_FPCR(type, name, insn, clear, set)			\
  type name(type x)							\
  {									\
    unsigned long fpcr;							\
    volatile type result;						\
    __asm__ __volatile__ ("fmove%.l %/fpcr,%0" : "=dm" (fpcr));		\
    __asm__ __volatile__ ("fmove%.l %0,%/fpcr"				\
			  : : "dmi" ((fpcr & ~(unsigned long) (clear)) | (set)));	\
    __asm__ __volatile__ (insn "%.x %1,%0" : "=f" (result) : "f" (x));	\
    __asm__ __volatile__ ("fmove%.l %0,%/fpcr" : : "dmi" (fpcr));	\
    return result;							\
  }

#define FPCR_PRECISION 0xc0
#define FPCR_SINGLE    0x40
#define FPCR_DOUBLE    0x80

#define SIGN_BIT 0x80000000UL

/* ceil() and floor() work on the bits, and round away from zero when x has the
   sign in away: fint isn't in a 68040's hardware, and it raises INEXACT, which
   C23's Annex F says these functions don't.  */
static inline double fpu_round_to_integer(double x, unsigned long away)
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

static inline float fpu_round_to_integerf(float x, unsigned long away)
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

#endif /* _FPU_MATH_H */
