/* scanf's conversion specifications, narrow and wide, with the results
   glibc gives: assignment suppression before the field width, and the
   integer conversions and %n storing the size of their length modifier,
   no more, and %u taking a sign.  Prints what fails; the exit code is the
   number of failures.  */

#include <stdio.h>
#include <string.h>
#include <wchar.h>

static int failures;

static void widen(wchar_t *w, const char *s)
{
  while ((*w++ = (unsigned char) *s++))
    ;
}

static void check(int ok, const char *fmt, const char *in, const char *what)
{
  if (!ok)
    {
      printf("FAIL: \"%s\" on \"%s\": %s\n", fmt, in, what);
      failures++;
    }
}

/* fmt converts one int after any suppressed fields */
static void suppressed(const char *fmt, const char *in, int r, int v)
{
  wchar_t wfmt[32], win[32];
  int x = -1, n;

  n = sscanf(in, fmt, &x);
  check(n == r && x == v, fmt, in, "sscanf");
  widen(wfmt, fmt);
  widen(win, in);
  x = -1;
  n = swscanf(win, wfmt, &x);
  check(n == r && x == v, fmt, in, "swscanf");
}

/* fmt stores want, of its size, in a buffer of guard bytes */
static void stored(const char *fmt, const char *in, const void *want, size_t size)
{
  wchar_t wfmt[32], win[32];
  unsigned char b[16], g[16];
  int pass;

  memset(g, 0xa5, sizeof(g));
  memcpy(g + 4, want, size);
  for (pass = 0; pass < 2; pass++)
    {
      memset(b, 0xa5, sizeof(b));
      if (!pass)
        sscanf(in, fmt, b + 4);
      else
        {
          widen(wfmt, fmt);
          widen(win, in);
          swscanf(win, wfmt, b + 4);
        }
      check(!memcmp(b, g, sizeof(b)), fmt, in, pass ? "swscanf" : "sscanf");
    }
}

#define STORED(fmt, in, type, v) \
  do { type w_ = (v); stored(fmt, in, &w_, sizeof(w_)); } while (0)

int main(void)
{
  STORED("%hhd", "-5", signed char, -5);
  STORED("%hhu", "200", unsigned char, 200);
  STORED("%hhx", "7f", unsigned char, 0x7f);
  STORED("%hd", "-300", short, -300);
  STORED("%d", "-70000", int, -70000);
  STORED("%ld", "-70000", long, -70000);
  STORED("%lld", "-1234567890123", long long, -1234567890123LL);
  STORED("%llx", "123456789abcdef0", unsigned long long, 0x123456789abcdef0ULL);
  STORED("%llu", "18446744073709551615", unsigned long long, 18446744073709551615ULL);
  STORED("%lli", "0x7fffffffffffffff", long long, 0x7fffffffffffffffLL);
  STORED("%jd", "-9000000000", long long, -9000000000LL);
  STORED("%zu", "4000000000", unsigned long, 4000000000UL);
  STORED("%td", "-7", long, -7);
  STORED("%u", "-1", unsigned int, 4294967295U);
  STORED("%hhu", "-1", unsigned char, 255);
  STORED("%llu", "-2", unsigned long long, 18446744073709551614ULL);
  STORED("%u", "+7", unsigned int, 7);
  STORED("abc%hhn", "abc", signed char, 3);
  STORED("abc%hn", "abc", short, 3);
  STORED("abc%n", "abc", int, 3);
  STORED("abc%lln", "abc", long long, 3);

  suppressed("%*3d%d", "12345", 1, 45);
  suppressed("%*d %d", "1 2", 1, 2);
  suppressed("%*2s%d", "ab7", 1, 7);
  suppressed("%*c%d", "x8", 1, 8);
  suppressed("%*[a-c]%d", "abc9", 1, 9);
  return failures;
}
