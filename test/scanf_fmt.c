/* scanf's conversion specifications, narrow and wide, with the results
   glibc gives: assignment suppression before the field width.  Prints what
   fails; the exit code is the number of failures.  */

#include <stdio.h>
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

int main(void)
{
  suppressed("%*3d%d", "12345", 1, 45);
  suppressed("%*d %d", "1 2", 1, 2);
  suppressed("%*2s%d", "ab7", 1, 7);
  suppressed("%*c%d", "x8", 1, 8);
  suppressed("%*[a-c]%d", "abc9", 1, 9);
  return failures;
}
