#include <math.h>

/* The sign of the last lgamma() result, which <math.h> reads as signgam
   through this function.  libnix has one per program, like errno.  */
static int signgam_value;

int *__signgam(void)
{
  return &signgam_value;
}
