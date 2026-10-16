#include <math.h>

/* libnix has no long double math: these functions compute in double,
   as libstdc++ did when libc lacked them.  */
long double modfl(long double x, long double *ip)
{
  double i;
  double f = modf(x, &i);
  *ip = i;
  return f;
}
