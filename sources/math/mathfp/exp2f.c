#include <math.h>
#include "math_errno.h"

/* exp2 in double, then rounded once to float.  */
float exp2f(float x)
{
  float result = (float)exp2(x);

#ifdef __MATH_SETS_ERRNO
  if (!__math_special_float(x)) {
    if (__math_special_float(result) || __math_zero_float(result))
      errno = ERANGE;
  }
#endif

  return result;
}
