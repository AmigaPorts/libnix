#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, log, "flogn")

#else

#include <proto/mathieeedoubtrans.h>

double log(double x)
{ return IEEEDPLog(x); }

#endif
