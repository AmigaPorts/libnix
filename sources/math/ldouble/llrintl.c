#include <math.h>
#include "ldbits.h"
#include "math_errno.h"

/* Exact for every result in range; out of range it is unspecified.  */
long long llrintl(long double x)
{
  long double rounded = rintl(x);

#ifdef __MATH_SETS_ERRNO
  if (!__ldbl_fits(rounded, __MATH_LONG_LONG_BITS))
    errno = EDOM;
#endif

  return (long long)rounded;
}
