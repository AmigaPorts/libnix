#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, logf, "flogn")

#else

#include <proto/mathieeesingtrans.h>

float logf(float x)
{ return IEEESPLog(x); }

#endif
