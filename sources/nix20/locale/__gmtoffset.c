/* The variables of tzset().  libnix has no time zones: Unix time is the
   system clock taken as UTC, for time(), file dates and localtime() alike,
   so the variables say UTC and tzset() has nothing to do.
 */
#include <time.h>

long __timezone__data = 0;
int  __daylight__data = 0;
char *__tzname__data[2] = {"GMT", "GMT"};

long * __timezone = &__timezone__data;
int  * __daylight = &__daylight__data;
char **__tzname = __tzname__data;

void tzset() {
}
