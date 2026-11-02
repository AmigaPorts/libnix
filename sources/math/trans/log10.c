#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, log10, "flog10")

#else

#include <proto/mathieeedoubtrans.h>

double log10(double x)
{ return IEEEDPLog10(x); }

#endif
