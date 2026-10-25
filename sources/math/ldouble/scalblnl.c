#include <limits.h>
#include <math.h>

/* scalbnl with a long exponent: any n beyond the int range overflows or
   underflows just the same.  */
long double scalblnl(long double x, long n)
{
  if (n > INT_MAX)
    n = INT_MAX;
  else if (n < INT_MIN)
    n = INT_MIN;
  return scalbnl(x, n);
}
