#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, cosf, "fcos")

#else

#include <proto/mathieeesingtrans.h>

float cosf(float x)
{ return IEEESPCos(x); }

#endif
