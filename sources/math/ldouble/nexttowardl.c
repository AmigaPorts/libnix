#include <math.h>

/* For long double, nexttoward is nextafter.  */
long double nexttowardl(long double x, long double y)
{
  return nextafterl(x, y);
}
