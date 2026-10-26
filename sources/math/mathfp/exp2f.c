#include <math.h>

/* exp2 in double, then rounded once to float.  */
float exp2f(float x)
{
  return (float)exp2(x);
}
