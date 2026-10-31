#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION_FPCR(float, sqrtf, "fsqrt", FPCR_PRECISION, FPCR_SINGLE)

#else

#include <proto/mathieeesingtrans.h>

float sqrtf(float x)
{ return IEEESPSqrt(x); }

#endif
