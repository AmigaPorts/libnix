#include <math.h>

/* log2 in double, then rounded once to float: the powers of 2 come out
   exact.  */
float log2f(float x)
{
  return (float)log2(x);
}
