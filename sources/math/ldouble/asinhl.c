#include <math.h>

/* libnix has no long double math: these functions compute in double,
   as libstdc++ did when libc lacked them.  */
long double asinhl(long double x)
{
  return asinh(x);
}
