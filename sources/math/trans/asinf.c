#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, asinf, "fasin")

#else

#include <proto/mathieeesingtrans.h>

float asinf(float x)
{ return IEEESPAsin(x); }

#endif
