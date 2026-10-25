#include <math.h>
#include "fdlibm.h"

/* pow(2, x), as newlib's mathfp exp2, but exact for the integers with a
   normal result, which pow need not be.  The |x| < 1024 test on the bits
   also keeps infinities and NaNs away from the comparisons, which
   -ffast-math may assume they never see.  */
double exp2(double x)
{
  __int32_t hx;
  int n;
  double r;

  GET_HIGH_WORD(hx, x);
  if ((hx & 0x7fffffff) < 0x40900000) {
    n = (int)x;
    if (n == x && n >= -1022) {
      INSERT_WORDS(r, (n + 1023) << 20, 0);
      return r;
    }
  }
  return pow(2.0, x);
}
