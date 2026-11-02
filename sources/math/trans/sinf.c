#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, sinf, "fsin")

#else

#include <proto/mathieeesingtrans.h>

float sinf(float x)
{ return IEEESPSin(x); }

#endif
