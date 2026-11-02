#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, sin, "fsin")

#else

#include <proto/mathieeedoubtrans.h>

double sin(double x)
{ return IEEEDPSin(x); }

#endif
