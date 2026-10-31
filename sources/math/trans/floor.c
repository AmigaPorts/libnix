#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION_FPCR(double, floor, "fint", FPCR_ROUNDING, FPCR_DOWNWARD)

#else

#include <proto/mathieeedoubbas.h>

double floor(double x)
{ return IEEEDPFloor(x); }

#endif
