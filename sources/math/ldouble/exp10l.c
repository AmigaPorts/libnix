#include <math.h>

extern double exp10(double x);

/* libnix has no long double math: these functions compute in double,
   as libstdc++ did when libc lacked them.  */
long double exp10l(long double x)
{
  return exp10(x);
}
