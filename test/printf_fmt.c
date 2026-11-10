/* printf's conversion specifications, narrow and wide, with the results
   glibc gives: %n stores the size of its length modifier, no more, the
   precision of %#x counts the digits, not the 0x, %a with a precision
   rounds, and widths and precisions go past a short's range.  Prints what
   fails; the exit code is the number of failures.  */

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

/* as printed(), with a double or a long double; libnix writes %a with a
   leading 1, where glibc may have 2 */
static void printed_d(const char *fmt, double v, const char *want)
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

static void printed_ld(const char *fmt, long double v, const char *want)
{
  char s[64];

  snprintf(s, sizeof(s), fmt, v);
  if (strcmp(s, want))
    printf("FAIL: \"%s\": got \"%s\", want \"%s\"\n", fmt, s, want), failures++;
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

  /* %a with a precision rounds to nearest, ties to even */
  printed_d("[%.2a]", 1.006, "[0x1.02p+0]");
  printed_d("[%.2a]", 1.001, "[0x1.00p+0]");
  printed_d("[%.2a]", 1.009, "[0x1.02p+0]");
  printed_d("[%.2a]", 1.999, "[0x1.00p+1]");
  printed_d("[%.0a]", 1.5, "[0x1p+1]");
  printed_d("[%.0a]", 1.25, "[0x1p+0]");
  printed_d("[%.1a]", 1.15625, "[0x1.2p+0]");
  printed_d("[%.1a]", 1.21875, "[0x1.4p+0]");
  printed_d("[%.1a]", 1.96875, "[0x1.0p+1]");
  printed_d("[%.3A]", -1.999, "[-0X1.FFCP+0]");
  printed_d("[%.2A]", -1.999, "[-0X1.00P+1]");
  printed_d("[%.0a]", 0x0.fffffffffffffp-1022, "[0x1p-1022]");
  printed_d("[%.13a]", 1.999, "[0x1.ffbe76c8b4396p+0]");
  printed_ld("[%.2La]", 1.999L, "[0x1.00p+1]");
  printed_ld("[%.0La]", 1.5L, "[0x1p+1]");
  printed_ld("[%.1La]", 1.15625L, "[0x1.2p+0]");

  /* widths and precisions past a short's range, by the length */
  check(snprintf(NULL, 0, "%40000d", 1) == 40000, "%40000d", "length");
  check(snprintf(NULL, 0, "%-70000d", 1) == 70000, "%-70000d", "length");
  check(snprintf(NULL, 0, "%*d", 70000, 1) == 70000, "%*d 70000", "length");
  check(snprintf(NULL, 0, "%*d", -70000, 1) == 70000, "%*d -70000", "length");
  check(snprintf(NULL, 0, "%.40000d", 1) == 40000, "%.40000d", "length");
  check(snprintf(NULL, 0, "%.*d", 70000, 1) == 70000, "%.*d 70000", "length");
  check(snprintf(NULL, 0, "%.20000f", 1.0) == 20002, "%.20000f", "length");
  check(snprintf(NULL, 0, "%.40000e", 1.0) == 40006, "%.40000e", "length");
  return failures;
}
