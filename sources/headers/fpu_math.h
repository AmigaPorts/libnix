/* With -m68881, the math functions use the FPU itself.  mathieeedoubtrans.library
   would only pass the result back: that takes longer, and it loses the exception
   flags and the rounding mode, which fenv.h is about.  */

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

/* The FPCR for a rounding mode or a precision for the time of one instruction:
   ceil() and floor() round up and down, and fsqrt in the precision of its result
   rounds only once.  */
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
#define FPCR_ROUNDING  0x30
#define FPCR_DOWNWARD  0x20
#define FPCR_UPWARD    0x30

#endif /* _FPU_MATH_H */
