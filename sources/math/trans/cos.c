#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, cos, "fcos")

#else

#include <proto/mathieeedoubtrans.h>

double cos(double x)
{ return IEEEDPCos(x); }

#endif
