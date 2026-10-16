#include <math.h>

/* libnix has no long double math: these functions compute in double,
   as libstdc++ did when libc lacked them.  */
long double fmodl(long double x, long double y)
{
  return fmod(x, y);
}
