#include <math.h>

/* Exact: clear the sign bit (also right for -0 and NaNs).  */
long double fabsl(long double x)
{
  union { long double v; unsigned short se; } u;

  u.v = x;
  u.se &= 0x7fff;
  return u.v;
}
