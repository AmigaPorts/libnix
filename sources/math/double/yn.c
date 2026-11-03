#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const double EULER_GAMMA =
    0.57721566490153286060651209;


/* Bessel Y0(x) */

double y0(double x)
{
    if (x < 0.0)
        return NAN;

    if (x == 0.0)
        return -HUGE_VAL;

    double xx = x * x;
    double term = 1.0;
    double h = 0.0;
    double sum = 0.0;

    for (int k = 1; k < 30; k++) {
        h += 1.0 / k;
        term *= -(xx / 4.0) / (k * k);
        sum += h * term;
    }

    return (2.0 / M_PI) *
           ((log(x / 2.0) + EULER_GAMMA) * j0(x)
            + sum);
}


/* Bessel Y1(x) */

double y1(double x)
{
    if (x < 0.0)
        return NAN;

    if (x == 0.0)
        return -HUGE_VAL;

    double xx = x * x;
    double term = x / 2.0;
    double h = 1.0;
    double sum = term;

    for (int k = 1; k < 30; k++) {
        term *= -(xx / 4.0) / (k * (k + 1));
        h += 1.0 / k + 1.0 / (k + 1);
        sum += h * term;
    }

    return (2.0 / M_PI) *
           ((log(x / 2.0) + EULER_GAMMA) * j1(x)
            - 1.0 / x
            + sum);
}


/* Bessel Yn(x), integer order */

double yn(int n, double x)
{
    double a, b, c;
    int i;
    int sign = 1;

    if (isnan(x))
        return x;

    if (x < 0.0)
        return NAN;

    if (x == 0.0)
        return -HUGE_VAL;

    if (n < 0) {
        n = -n;

        /* Y(-n,x) = (-1)^n Y(n,x) */

        if (n & 1)
            sign = -1;
    }

    if (n == 0)
        return sign * y0(x);

    if (n == 1)
        return sign * y1(x);

    a = y0(x);
    b = y1(x);

    for (i = 1; i < n; i++) {
        c = (2.0 * i / x) * b - a;
        a = b;
        b = c;
    }

    return sign * b;
}
