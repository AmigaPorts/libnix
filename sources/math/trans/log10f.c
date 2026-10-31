#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, log10f, "flog10")

#else

#include <proto/mathieeesingtrans.h>

float log10f(float x)
{ return IEEESPLog10(x); }

#endif
