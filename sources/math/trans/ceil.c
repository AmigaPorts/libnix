#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION_FPCR(double, ceil, "fint", FPCR_ROUNDING, FPCR_UPWARD)

#else

#include <proto/mathieeedoubbas.h>

double ceil(double x)
{ return IEEEDPCeil(x); }

#endif
