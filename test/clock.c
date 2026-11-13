/* libnix has no time zones: Unix time is the system clock taken as UTC.
   time() and gettimeofday() are DateStamp() since 1970, whatever timezone
   holds and whatever setlocale() is asked; localtime() is gmtime() and
   mktime() undoes it; a new file's date is the clock's, and utime() stores
   the date as the DateStamp it is.  clock_gettime() reads the same clock
   as CLOCK_REALTIME and timer.device's E-clock as CLOCK_MONOTONIC.  */

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <time.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <devices/timer.h>
#include <proto/dos.h>
#include <proto/timer.h>

#define TimerBase (DOSBase->dl_TimeReq->tr_node.io_Device)

#define NAME "T:libnix-clock-test"

/* 1978-01-01 in seconds since 1970-01-01 */
#define AMIGA_EPOCH 252460800L

static int failures;
static const char *stage = "";

static void check(int ok, const char *what)
{
  if (!ok)
    {
      printf("FAIL: %s%s\n", what, stage);
      failures++;
    }
}

/* DS in Unix time.  */
static long unixtime(const struct DateStamp *ds)
{
  return (ds->ds_Days * 1440L + ds->ds_Minute) * 60 + ds->ds_Tick / TICKS_PER_SECOND
    + AMIGA_EPOCH;
}

static long now(void)
{
  struct DateStamp ds;

  DateStamp(&ds);
  return unixtime(&ds);
}

static void clocks(void)
{
  struct timeval tv;
  struct timespec ts = { -1, -1 };
  long before, after;
  time_t t;

  before = now();
  t = time(NULL);
  gettimeofday(&tv, NULL);
  clock_gettime(CLOCK_REALTIME, &ts);
  after = now();
  check(t >= before && t <= after, "time() is the DateStamp since 1970");
  check(tv.tv_sec >= before && tv.tv_sec <= after, "gettimeofday() too");
  check(tv.tv_usec >= 0 && tv.tv_usec < 1000000, "with microseconds below a second");
  check(ts.tv_sec >= before && ts.tv_sec <= after, "clock_gettime(CLOCK_REALTIME) too");
  check(ts.tv_nsec >= 0 && ts.tv_nsec < 1000000000, "with nanoseconds below a second");
}

/* The E-clock counter in nanoseconds, and its frequency in *FREQ.  */
static long long eclock(unsigned long *freq)
{
  struct EClockVal ev;
  unsigned long long ticks;

  *freq = ReadEClock(&ev);
  ticks = (unsigned long long) ev.ev_hi << 32 | ev.ev_lo;
  return ticks / *freq * 1000000000 + ticks % *freq * 1000000000 / *freq;
}

static void monotonic(void)
{
  struct timespec ts = { -1, -1 }, res = { -1, -1 };
  long long before, after, t;
  unsigned long freq;

  check(clock_getres(CLOCK_REALTIME, &res) == 0 && res.tv_sec == 0
        && res.tv_nsec == 1000000000 / TICKS_PER_SECOND, "clock_getres(CLOCK_REALTIME) says a tick");
  before = eclock(&freq);
  check(clock_gettime(CLOCK_MONOTONIC, &ts) == 0, "clock_gettime(CLOCK_MONOTONIC) succeeds");
  after = eclock(&freq);
  t = ts.tv_sec * 1000000000LL + ts.tv_nsec;
  check(ts.tv_nsec >= 0 && ts.tv_nsec < 1000000000, "with nanoseconds below a second");
  /* to an E-clock tick, which the conversion may round away */
  check(t >= before - 2000 && t <= after + 2000, "and is the E-clock counter");
  check(clock_getres(CLOCK_MONOTONIC, &res) == 0 && res.tv_sec == 0
        && res.tv_nsec == 1000000000 / freq, "clock_getres(CLOCK_MONOTONIC) says an E-clock tick");
  errno = 0;
  check(clock_gettime((clockid_t) 99, &ts) == -1 && errno == EINVAL,
        "a clock there isn't fails with EINVAL");
}

int main(void)
{
  struct FileInfoBlock *fib;
  struct utimbuf ut;
  struct stat st;
  struct tm tm;
  long before, after;
  time_t t;
  BPTR fh, lock;

  /* an hour east of UTC, which nothing may apply */
  _timezone = -3600;

  clocks();
  setlocale(LC_ALL, "C");
  stage = ", after setlocale(LC_ALL, \"C\")";
  clocks();
  setlocale(LC_ALL, "");
  stage = ", after setlocale(LC_ALL, \"\")";
  clocks();
  stage = "";
  monotonic();

  t = time(NULL);
  tm = *localtime(&t);
  check(tm.tm_hour * 60 + tm.tm_min == t / 60 % 1440, "localtime() is gmtime()");
  check(mktime(&tm) == t, "mktime() undoes localtime()");

  before = now();
  if ((fh = Open((STRPTR) NAME, MODE_NEWFILE)))
    Close(fh);
  after = now();
  check(fh != 0, "create a file");
  check(stat(NAME, &st) == 0, "stat() on the new file succeeds");
  check(st.st_mtime >= before - 2 && st.st_mtime <= after + 2, "its date is the clock's");

  ut.actime = ut.modtime = 1000000000;
  check(utime(NAME, &ut) == 0, "utime() succeeds");
  check(stat(NAME, &st) == 0 && st.st_mtime == 1000000000, "stat() reads the date back");
  fib = AllocDosObject(DOS_FIB, NULL);
  lock = Lock((STRPTR) NAME, SHARED_LOCK);
  check(fib && lock && Examine(lock, fib) && unixtime(&fib->fib_Date) == 1000000000,
        "the file's DateStamp is that date");
  if (lock)
    UnLock(lock);
  FreeDosObject(DOS_FIB, fib);
  DeleteFile((STRPTR) NAME);

  printf("%d failures\n", failures);
  return failures;
}
