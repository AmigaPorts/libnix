#include <math.h>

/* Exact: rintl, which raises no exception libnix reports.  */
long double nearbyintl(long double x)
{
  return rintl(x);
}
