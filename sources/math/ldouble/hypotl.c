#include <math.h>

/* libnix has no long double math: these functions compute in double,
   as libstdc++ did when libc lacked them.  */
long double hypotl(long double x, long double y)
{
  return hypot(x, y);
}
