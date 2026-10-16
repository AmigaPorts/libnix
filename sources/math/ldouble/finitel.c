#include <math.h>

/* Exact: a long double beyond the double range is still finite.  */
int finitel(long double x)
{
  union { long double v; unsigned short se; } u;

  u.v = x;
  return (u.se & 0x7fff) != 0x7fff;
}
