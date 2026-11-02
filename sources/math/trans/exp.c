#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, exp, "fetox")

#else

#include <proto/mathieeedoubtrans.h>

double exp(double x)
{ return IEEEDPExp(x); }

#endif
