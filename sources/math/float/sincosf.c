#include <math.h>
void sincos(float x, float *sinx, float *cosx) {
	double d = x;
    *sinx = (float)sin(d);
    *cosx = (float)cos(d);
}
