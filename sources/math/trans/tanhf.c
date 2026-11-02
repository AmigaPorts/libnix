#ifdef __HAVE_68881__

#include "fpu_math.h"

FPU_FUNCTION(float, tanhf, "ftanh")

#else

#include <proto/mathieeesingtrans.h>

float tanhf(float x)
{ return IEEESPTanh(x); }

#endif
