#include <math.h>

/* libnix has no long double math: these functions compute in double,
   as libstdc++ did when libc lacked them.  */
long double remquol(long double x, long double y, int *quo)
{
  return remquo(x, y, quo);
}
