#include <math.h>

/* libnix has no long double math: these functions compute in double,
   as libstdc++ did when libc lacked them.  lgamma() sets signgam.  */
long double lgammal(long double x)
{
  return lgamma(x);
}
