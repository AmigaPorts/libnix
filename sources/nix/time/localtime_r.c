#include <time.h>

/* The system clock is taken as UTC, so local time is gmtime_r()'s.  */
struct tm *localtime_r(const time_t *t, struct tm *result)
{
  return gmtime_r(t, result);
}
