/* printf() of floating point: a precision or a value that asks for long
   runs of zeros (they once overran a stack buffer), a negative precision
   from '*' (meaning no precision), the field width around such runs, and
   sprintf() terminating an empty output.  Prints what fails; the exit code
   is the number of failures.  */

#include <stdio.h>
#include <string.h>

static int failures;
static char buf[4200];

static void check(int ok, const char *what)
{
  if (!ok)
    {
      printf("FAIL: %s\n", what);
      failures++;
    }
}

/* n characters: prefix, then only zeros up to suffix */
static int zeros_between(const char *prefix, const char *suffix, int n)
{
  size_t lp = strlen(prefix), ls = strlen(suffix);
  int i;

  if ((int)strlen(buf) != n || strncmp(buf, prefix, lp)
      || strcmp(buf + n - ls, suffix))
    return 0;
  for (i = lp; i < n - (int)ls; i++)
    if (buf[i] != '0')
      return 0;
  return 1;
}

int main(void)
{
  volatile double one = 1.0, big = 1e300, tiny = 1e-300, x = 1.23456789;
  int n;

  n = sprintf(buf, "%.4000e", one);
  check(n == 4006 && zeros_between("1.", "e+00", 4006), "%.4000e");

  n = sprintf(buf, "%.300f", tiny);
  check(n == 302 && zeros_between("0.", "1", 302), "%.300f of 1e-300");

  n = sprintf(buf, "%f", big);
  check(n == 308 && buf[0] == '1' && !strcmp(buf + 301, ".000000"), "%f of 1e300");

  n = sprintf(buf, "%.200a", one);
  check(n == 207 && zeros_between("0x1.", "p+0", 207), "%.200a");

  n = sprintf(buf, "%25f", 1e20);
  check(n == 28 && !strcmp(buf, "100000000000000000000.000000"), "%25f of 1e20");

  n = sprintf(buf, "%30f|", 1e20);
  check(n == 31 && !strcmp(buf, "  100000000000000000000.000000|"), "%30f of 1e20");

  n = sprintf(buf, "%-30f|", 1e20);
  check(n == 31 && !strcmp(buf, "100000000000000000000.000000  |"), "%-30f of 1e20");

  n = sprintf(buf, "%012.3f", -0.000125);
  check(n == 12 && !strcmp(buf, "-0000000.000"), "%012.3f of -0.000125");

  n = sprintf(buf, "%.*a", -1, x);
  check(n == 20 && !strcmp(buf, "0x1.3c0ca4283de1bp+0"), "%.*a with precision -1");

  n = sprintf(buf, "%.*f", -1, one);
  check(n == 8 && !strcmp(buf, "1.000000"), "%.*f with precision -1");

  strcpy(buf, "stale");
  n = sprintf(buf, "%s", "");
  check(n == 0 && buf[0] == '\0', "sprintf of an empty string");

  printf("%d failures\n", failures);
  return failures;
}
