#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION_FPCR(float, ceilf, "fint", FPCR_ROUNDING, FPCR_UPWARD)

#else

#include <proto/mathieeesingbas.h>

float ceilf(float x)
{ return IEEESPCeil(x); }

#endif
