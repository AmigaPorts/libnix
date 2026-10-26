#include <math.h>

/* Exact: halfway cases away from zero.  x - truncl(x) is exact, and so
   is adding 1 to an integer below 2^63.  */
long double roundl(long double x)
{
  long double t = truncl(x);
  long double d = x - t;
  if (d >= 0.5L)
    t += 1.0L;
  else if (d <= -0.5L)
    t -= 1.0L;
  return t;
}
