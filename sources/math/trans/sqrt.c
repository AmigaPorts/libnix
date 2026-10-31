#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION_FPCR(double, sqrt, "fsqrt", FPCR_PRECISION, FPCR_DOUBLE)

#else

#include <proto/mathieeedoubtrans.h>

double sqrt(double x)
{ return IEEEDPSqrt(x); }

#endif
