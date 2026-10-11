#include <proto/mathieeesingtrans.h>
#include <stdint.h>

#define PI 3.14159265358979323846

static inline float atanf(float x)
{ return IEEESPAtan(x); }

float atan2f(float y,float x)
{
  union { float f; uint32_t bits; } ux, uy;
  ux.f = x;
  uy.f = y;

  /* Avoid 0/0 and preserve signed zero, including when built with
     -ffast-math. Leave NaN arguments to the existing calculation. */
  if ((uy.bits & 0x7fffffffU) == 0 &&
      (ux.bits & 0x7fffffffU) <= 0x7f800000U) {
    if (ux.bits & 0x80000000U)
      return (uy.bits & 0x80000000U) ? -PI : PI;
    return y;
  }

  return x>=y?(x>=-y?      atanf(y/x):     -PI/2-atanf(x/y)):
              (x>=-y? PI/2-atanf(x/y):y>=0? PI  +atanf(y/x):
                                          -PI  +atanf(y/x));
}
