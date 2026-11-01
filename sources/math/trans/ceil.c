#ifdef __HAVE_68881__

#include "fpu_math.h"

double ceil(double x)
{ return fpu_round_to_integer(x, 0); }

#else

#include <proto/mathieeedoubbas.h>

double ceil(double x)
{ return IEEEDPCeil(x); }

#endif
