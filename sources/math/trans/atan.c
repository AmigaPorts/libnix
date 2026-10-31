#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, atan, "fatan")

#else

#include <proto/mathieeedoubtrans.h>

double atan(double x)
{ return IEEEDPAtan(x); }

#endif
