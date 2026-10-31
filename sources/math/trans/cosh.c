#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, cosh, "fcosh")

#else

#include <proto/mathieeedoubtrans.h>

double cosh(double x)
{ return IEEEDPCosh(x); }

#endif
