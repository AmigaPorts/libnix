#ifdef __HAVE_68881__

#include "fpu_math.h"

float ceilf(float x)
{ return fpu_round_to_integerf(x, 0); }

#else

#include <proto/mathieeesingbas.h>

float ceilf(float x)
{ return IEEESPCeil(x); }

#endif
