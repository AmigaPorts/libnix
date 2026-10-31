#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, tan, "ftan")

#else

#include <proto/mathieeedoubtrans.h>

double tan(double x)
{ return IEEEDPTan(x); }

#endif
