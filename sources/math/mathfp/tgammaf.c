#include <errno.h>
#include <float.h>
#include <math.h>

/* tgamma in double, which is more accurate than lgammaf would be, and
   ERANGE when the result overflows a float.  */
float tgammaf(float x)
{
  double y = tgamma(x);
  if (y > FLT_MAX || y < -FLT_MAX)
    errno = ERANGE;
  return (float)y;
}
