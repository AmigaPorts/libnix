#include <math.h>
#include "fdlibm.h"
#include "../ldouble/ldbits.h"

/* nextafter towards a long double.  x and y are compared as long
   doubles, so a y just past x, closer than the next double, still moves
   x.  NaNs are found on the bits, since libnix is built with
   -ffast-math.  */
double nexttoward(double x, long double y)
{
  __int32_t hx;
  __uint32_t lx;

  EXTRACT_WORDS(hx, lx, x);
  if (((hx & 0x7ff00000) == 0x7ff00000 && ((hx & 0xfffff) | lx)) || __ldbl_isnan(y))
    return x + y;
  if ((long double)x == y)
    return y;
  return nextafter(x, (long double)x < y ? HUGE_VAL : -HUGE_VAL);
}
