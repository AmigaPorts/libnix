#include <math.h>

/* Exact for every result in range; out of range it is unspecified.  */
long lroundl(long double x)
{
  return (long)roundl(x);
}
