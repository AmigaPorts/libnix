#include <math.h>

/* Exact for every result in range; out of range it is unspecified.  */
long long llroundl(long double x)
{
  return (long long)roundl(x);
}
