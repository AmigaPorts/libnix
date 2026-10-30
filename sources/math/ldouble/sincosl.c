#include <math.h>
void sincosl(long double x, long double *sinx, long double *cosx) {
	double d = x;
    *sinx = (long double)sin(d);
    *cosx = (long double)cos(d);
}
