#ifdef __HAVE_68881__

#include "fpu_math.h"

float floorf(float x)
{ return fpu_round_to_integerf(x, SIGN_BIT); }

#else

#include <proto/mathieeesingbas.h>

float floorf(float x)
{ return IEEESPFloor(x); }

#endif
