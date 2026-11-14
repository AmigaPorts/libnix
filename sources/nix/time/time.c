#include <time.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include "amigatime.h"

time_t time(time_t *tloc)
{ struct DateStamp t;
  time_t ti;
  DateStamp(&t); /* Get timestamp */
  ti=(t.ds_Days*1440+t.ds_Minute)*60+
     t.ds_Tick/TICKS_PER_SECOND+AMIGA_EPOCH;
  if(tloc!=NULL)
    *tloc=ti;
  return ti;
}

/*
 * 1440 is the number of minutes per day
 *   60 is the number of seconds per minute
 */
