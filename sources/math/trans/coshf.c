#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, coshf, "fcosh")

#else

#include <proto/mathieeesingtrans.h>

float coshf(float x)
{ return IEEESPCosh(x); }

#endif
