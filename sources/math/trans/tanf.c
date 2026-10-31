#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, tanf, "ftan")

#else

#include <proto/mathieeesingtrans.h>

float tanf(float x)
{ return IEEESPTan(x); }

#endif
