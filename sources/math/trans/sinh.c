#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, sinh, "fsinh")

#else

#include <proto/mathieeedoubtrans.h>

double sinh(double x)
{ return IEEEDPSinh(x); }

#endif
