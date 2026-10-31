#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, acos, "facos")

#else

#include <proto/mathieeedoubtrans.h>

double acos(double x)
{ return IEEEDPAcos(x); }

#endif
