/* Host substitutes for the Amiga library calls; keep these free of
   fast-math and symbol renaming so atan resolves to the host libm. */
#include <math.h>
#include "proto/mathieeesingtrans.h"
#include "proto/mathieeedoubtrans.h"

float IEEESPAtan(float x)
{ return (float)atan((double)x); }

double IEEEDPAtan(double x)
{ return atan(x); }
