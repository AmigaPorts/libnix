#include <time.h>
#include <proto/dos.h>
#include <proto/timer.h>

#define TimerBase DOSBase->dl_TimeReq->tr_node.io_Device

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    if (tp) {
        struct DateStamp stamp;
        DateStamp(&stamp);

        // Whole seconds from ticks
        long sec_from_ticks = stamp.ds_Tick / TICKS_PER_SECOND;

        // Ticks within the current second
        long ticks_in_sec = stamp.ds_Tick % TICKS_PER_SECOND;

        // Convert Amiga date to UNIX epoch
        long days = stamp.ds_Days;
        long minutes = stamp.ds_Minute;

        // Amiga epoch (Jan 1 1978) to UNIX epoch (Jan 1 1970)
        const long AMIGA_TO_UNIX = 252460800;

        // Seconds since UNIX epoch
        tp->tv_sec =
            (days * 24 * 60 + minutes) * 60
            + sec_from_ticks
            + AMIGA_TO_UNIX;

        // Fractional nanoseconds
        // ticks_in_sec / 50 ->fraction of a second
        // multiply by 20,000,000 to get nanoseconds
        tp->tv_nsec = (ticks_in_sec * (1000000000 / TICKS_PER_SECOND));
    }
    return 0;
}

