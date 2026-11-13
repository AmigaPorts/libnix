/* gettimeofday(): the DateStamp() clock, which counts from 1978-01-01, taken
   as UTC.  */

#include <sys/time.h>
#include <dos/dos.h>
#include <proto/dos.h>

/* 1978-01-01 in seconds since 1970-01-01 */
#define AMIGA_EPOCH 252460800L

int gettimeofday(struct timeval *tv, struct timezone *tz) {
	if (tv) {
		struct DateStamp ds;
		unsigned short ticks, secs;

		DateStamp(&ds);
		/* under 3000: a 16-bit division, which the 68000 has */
		ticks = ds.ds_Tick;
		secs = ticks / TICKS_PER_SECOND;
		tv->tv_sec = (ds.ds_Days * 1440L + ds.ds_Minute) * 60L + secs + AMIGA_EPOCH;
		tv->tv_usec = (unsigned short) (ticks - secs * TICKS_PER_SECOND)
			* (1000000L / TICKS_PER_SECOND);
	}
	if (tz) {
		tz->tz_minuteswest = 0;
		tz->tz_dsttime = 0;
	}
	return 0;
}
