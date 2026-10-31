#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, sinhf, "fsinh")

#else

#include <proto/mathieeesingtrans.h>

float sinhf(float x)
{ return IEEESPSinh(x); }

#endif
