#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, expf, "fetox")

#else

#include <proto/mathieeesingtrans.h>

float expf(float x)
{ return IEEESPExp(x); }

#endif
