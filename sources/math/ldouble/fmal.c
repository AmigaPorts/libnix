#include <math.h>

/* libnix has no long double math: these functions compute in double,
   as libstdc++ did when libc lacked them.  */
long double fmal(long double x, long double y, long double z)
{
  return fma(x, y, z);
}
