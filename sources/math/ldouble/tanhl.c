#include <math.h>

/* libnix has no long double math: these functions compute in double,
   as libstdc++ did when libc lacked them.  */
long double tanhl(long double x)
{
  return tanh(x);
}
