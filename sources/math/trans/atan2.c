#include <proto/mathieeedoubtrans.h>
#include <stdint.h>

#define PI 3.14159265358979323846

static inline double atan(double x)
{ return IEEEDPAtan(x); }

double atan2(double y,double x)
{
  union { double f; uint64_t bits; } ux, uy;
  ux.f = x;
  uy.f = y;

  /* Avoid 0/0 and preserve signed zero, including when built with
     -ffast-math. Leave NaN arguments to the existing calculation. */
  if ((uy.bits & 0x7fffffffffffffffULL) == 0 &&
      (ux.bits & 0x7fffffffffffffffULL) <= 0x7ff0000000000000ULL) {
    if (ux.bits & 0x8000000000000000ULL)
      return (uy.bits & 0x8000000000000000ULL) ? -PI : PI;
    return y;
  }

  return x>=y?(x>=-y?      atan(y/x):     -PI/2-atan(x/y)):
              (x>=-y? PI/2-atan(x/y):y>=0? PI  +atan(y/x):
                                          -PI  +atan(y/x));
}
