#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, atanf, "fatan")

#else

#include <proto/mathieeesingtrans.h>

float atanf(float x)
{ return IEEESPAtan(x); }

#endif
