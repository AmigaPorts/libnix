#ifdef __HAVE_68881__

#include "fpu_math.h"

double floor(double x)
{ return fpu_round_to_integer(x, SIGN_BIT); }

#else

#include <proto/mathieeedoubbas.h>

double floor(double x)
{ return IEEEDPFloor(x); }

#endif
