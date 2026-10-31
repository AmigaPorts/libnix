#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION_FPCR(float, floorf, "fint", FPCR_ROUNDING, FPCR_DOWNWARD)

#else

#include <proto/mathieeesingbas.h>

float floorf(float x)
{ return IEEESPFloor(x); }

#endif
