#include <math.h>

/* Exact for every result in range; out of range it is unspecified.  */
long lrintl(long double x)
{
  return (long)rintl(x);
}
