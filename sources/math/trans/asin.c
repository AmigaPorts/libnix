#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, asin, "fasin")

#else

#include <proto/mathieeedoubtrans.h>

double asin(double x)
{ return IEEEDPAsin(x); }

#endif
