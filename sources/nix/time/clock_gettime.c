/* clock_gettime() and clock_getres().  CLOCK_REALTIME is gettimeofday()'s
   clock.  CLOCK_MONOTONIC counts timer.device's E-clock, which setting the
   time does not move; before Kickstart 2.0, which has no ReadEClock(), it
   is the realtime clock too.  */

#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <devices/timer.h>
#include <proto/dos.h>
#include <proto/timer.h>

/* dos.library's own timer request, open for as long as the program runs */
#define TimerBase (DOSBase->dl_TimeReq->tr_node.io_Device)

/* 0 for the realtime clock, 1 for the E-clock, -1 for a clock there isn't.  */
static int kind(clockid_t clk_id) {
	if (clk_id == CLOCK_REALTIME)
		return 0;
	if (clk_id == CLOCK_MONOTONIC)
		return TimerBase->dd_Library.lib_Version >= 36;
	errno = EINVAL;
	return -1;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
	int k = kind(clk_id);

	if (k < 0)
		return -1;
	if (k) {
		struct EClockVal ev;
		unsigned long freq = ReadEClock(&ev), secs = 0, rem = 0, nsec = 0;
		int i;

		/* 64 bits by 32, a bit at a time: libgcc's division is 2.5 KB */
		for (i = 0; i < 64; i++) {
			rem = rem << 1 | ev.ev_hi >> 31;
			ev.ev_hi = ev.ev_hi << 1 | ev.ev_lo >> 31;
			ev.ev_lo <<= 1;
			secs <<= 1;
			if (rem >= freq) {
				rem -= freq;
				secs++;
			}
		}
		/* three decimal digits at a time, which fit in 32 bits below 4 MHz */
		for (i = 0; i < 3; i++) {
			rem *= 1000;
			nsec = nsec * 1000 + rem / freq;
			rem %= freq;
		}
		tp->tv_sec = secs;
		tp->tv_nsec = nsec;
	} else {
		struct timeval tv;

		gettimeofday(&tv, 0);
		tp->tv_sec = tv.tv_sec;
		tp->tv_nsec = tv.tv_usec * 1000;
	}
	return 0;
}

int clock_getres(clockid_t clk_id, struct timespec *res) {
	int k = kind(clk_id);

	if (k < 0)
		return -1;
	if (res) {
		struct EClockVal ev;

		res->tv_sec = 0;
		res->tv_nsec = 1000000000UL / (k ? ReadEClock(&ev) : TICKS_PER_SECOND);
	}
	return 0;
}
