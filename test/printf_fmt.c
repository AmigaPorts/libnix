/* printf's conversion specifications, narrow and wide, with the results
   glibc gives: %n stores the size of its length modifier, no more, and the
   precision of %#x counts the digits, not the 0x.  Prints what fails; the
   exit code is the number of failures.  */

#include <stdio.h>
#include <string.h>
#include <wchar.h>

static int failures;

static void widen(wchar_t *w, const char *s)
{
  while ((*w++ = (unsigned char) *s++))
    ;
}

static void check(int ok, const char *fmt, const char *what)
{
  if (!ok)
    {
      printf("FAIL: \"%s\": %s\n", fmt, what);
      failures++;
    }
}

/* fmt prints 3 characters, then its %n stores want, of its size, in a
   buffer of guard bytes */
static void stored(const char *fmt, const void *want, size_t size)
{
  char s[16];
  wchar_t wfmt[16], ws[16];
  unsigned char b[16], g[16];
  int pass;

  memset(g, 0xa5, sizeof(g));
  memcpy(g + 4, want, size);
  for (pass = 0; pass < 2; pass++)
    {
      memset(b, 0xa5, sizeof(b));
      if (!pass)
        snprintf(s, sizeof(s), fmt, b + 4);
      else
        {
          widen(wfmt, fmt);
          swprintf(ws, 16, wfmt, b + 4);
        }
      check(!memcmp(b, g, sizeof(b)), fmt, pass ? "swprintf" : "snprintf");
    }
}

#define STORED(fmt, type, v) \
  do { type w_ = (v); stored(fmt, &w_, sizeof(w_)); } while (0)

/* fmt, with one int, prints want, as glibc does */
static void printed(const char *fmt, int v, const char *want)
{
  char s[64];
  wchar_t wfmt[32], ws[64], wwant[64];

  snprintf(s, sizeof(s), fmt, v);
  check(!strcmp(s, want), fmt, "snprintf");
  widen(wfmt, fmt);
  widen(wwant, want);
  swprintf(ws, 64, wfmt, v);
  check(!wcscmp(ws, wwant), fmt, "swprintf");
}

int main(void)
{
  STORED("abc%hhn", signed char, 3);
  STORED("abc%hn", short, 3);
  STORED("abc%n", int, 3);
  STORED("abc%ln", long, 3);
  STORED("abc%lln", long long, 3);
  STORED("abc%jn", long long, 3);
  STORED("abc%zn", unsigned long, 3);
  STORED("abc%tn", long, 3);

  printed("[%#.5x]", 1, "[0x00001]");
  printed("[%#.1x]", 1, "[0x1]");
  printed("[%#.0x]", 1, "[0x1]");
  printed("[%#08.3x]", 1, "[   0x001]");
  printed("[%#.5o]", 1, "[00001]");
  printed("[%#10.4X]", 255, "[    0X00FF]");
  return failures;
}
