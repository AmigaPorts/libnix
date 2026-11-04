#include <math.h>
void sincosf(float x, float *sinx, float *cosx) {
	double d = x;
    *sinx = (float)sin(d);
    *cosx = (float)cos(d);
}
