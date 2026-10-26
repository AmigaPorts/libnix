#include <errno.h>
#include <math.h>
#include "fdlibm.h"

/* exp(lgamma(x)) with the sign of the gamma function, as newlib's mathfp
   tgamma, and C99's errors: a pole at +-0 (ERANGE, +-HUGE_VAL), a domain
   error at the negative integers and -inf (EDOM, NaN), and overflow
   (ERANGE, HUGE_VAL).  Infinities and NaNs are found on the bits, since
   libnix is built with -ffast-math.  */
double tgamma(double x)
{
  __int32_t hx;
  __uint32_t lx;
  int sign;
  double y;

  EXTRACT_WORDS(hx, lx, x);
  if (((hx & 0x7fffffff) | lx) == 0) {
    errno = ERANGE;
    return hx < 0 ? -HUGE_VAL : HUGE_VAL;
  }
  if ((hx & 0x7ff00000) == 0x7ff00000) {
    if (hx == (__int32_t)0xfff00000 && lx == 0) {
      errno = EDOM;
      return nan("");
    }
    return x + x;                       /* +inf, or the NaN */
  }
  if (hx < 0 && floor(x) == x) {
    errno = EDOM;
    return nan("");
  }
  if (x > 171.62437695630272) {         /* the largest x with a finite result */
    errno = ERANGE;
    return HUGE_VAL;
  }
  y = exp(lgamma_r(x, &sign));
  return sign < 0 ? -y : y;
}
