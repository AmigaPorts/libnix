#include <math.h>
long double lgammal_r(long double x, int *signp) {
    return (long double)lgamma_r((double)x, signp);
}
