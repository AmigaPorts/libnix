#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double yn(int n, double x) {
    if (x <= 0.0) return -HUGE_VAL;

    double sum = 0.0;
    int steps = 180;
    double dt = M_PI / steps;
    int i;

    for (i = 0; i <= steps; i++) {
        double t = i * dt;
        double factor = (i == 0 || i == steps) ? 1.0 : ((i % 2 == 0) ? 2.0 : 4.0);
        sum += factor * sin(x * sin(t) - n * t);
    }
    return (dt / 3.0) * sum / M_PI;
}

double y0(double x) {
	return yn(0, x);
}

double y1(double x) {
	return yn(1, x);
}
