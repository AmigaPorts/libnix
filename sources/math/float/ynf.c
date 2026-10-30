#include <math.h>

float ynf(int n, float x) {
    return (float)yn((int)n, (double)x);
}

float y0f(float x) {
	return ynf(0, x);
}

float y1f(float x) {
	return ynf(1, x);
}
