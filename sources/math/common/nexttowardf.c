#include <math.h>
#include "fdlibm.h"
#include "../ldouble/ldbits.h"

/* nextafterf towards a long double; see nexttoward.  */
float nexttowardf(float x, long double y)
{
  __int32_t ix;

  GET_FLOAT_WORD(ix, x);
  if ((ix & 0x7fffffff) > 0x7f800000 || __ldbl_isnan(y))
    return x + y;
  if ((long double)x == y)
    return y;
  return nextafterf(x, (long double)x < y ? HUGE_VALF : -HUGE_VALF);
}
