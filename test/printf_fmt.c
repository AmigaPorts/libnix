/* printf's conversion specifications, narrow and wide: %n stores the size
   of its length modifier, no more.  Prints what fails; the exit code is the
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
  return failures;
}
