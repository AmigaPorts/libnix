#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, acosf, "facos")

#else

#include <proto/mathieeesingtrans.h>

float acosf(float x)
{ return IEEESPAcos(x); }

#endif
