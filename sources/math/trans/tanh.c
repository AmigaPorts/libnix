#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(double, tanh, "ftanh")

#else

#include <proto/mathieeedoubtrans.h>

double tanh(double x)
{ return IEEEDPTanh(x); }

#endif
