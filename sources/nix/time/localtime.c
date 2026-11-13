#include <time.h>

/* The system clock is taken as UTC, so local time is gmtime()'s.  */
struct tm *localtime(const time_t *t)
{
  return gmtime(t);
}
