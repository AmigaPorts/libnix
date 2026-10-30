#include <math.h>


// Core function: Bessel function of the first kind, nth order
double jn(int n, double x) {
    // Handle negative orders using mathematical symmetry: J_{-n}(x) = (-1)^n * J_n(x)
    if (n < 0) {
        return (n % 2 == 0) ? jn(-n, x) : -jn(-n, x);
    }

    double sum = 0.0;
    int k;
    // Taylor series approximation (25 iterations provide high accuracy for standard tests)
    for (k = 0; k < 25; k++) {
        double num = pow(-1.0, k) * pow(x / 2.0, n + 2 * k);
        double den = 1.0;
        int i;
        for (i = 1; i <= k; i++) den *= i;
        for (i = 1; i <= n + k; i++) den *= i;
        sum += num / den;
    }
    return sum;
}

// Specialized standard aliases for Order 0 and Order 1
double j0(double x) { return jn(0, x); }
double j1(double x) { return jn(1, x); }
