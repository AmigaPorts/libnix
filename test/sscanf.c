/* sscanf(): conversions that read one character ahead and push it back
   with ungetc() must not end the string.  A literal after a number
   ("%d.%d"), a field width ("%02d%02d"), a literal prefix ("REM %d")
   and plain whitespace separators, as FTP replies, cheat codes and
   CUE sheets use them.  Prints what fails; the exit code is the
   number of failures.  */

#include <stdio.h>

static int failures;

static void check(int ok, const char *what)
{
  if (!ok)
    {
      printf("FAIL: %s\n", what);
      failures++;
    }
}

int main(void)
{
  int a = -1, b = -1, c = -1, d = -1, n;
  short y = -1, mo = -1, da = -1, h = -1, mi = -1, s = -1;
  unsigned long x = 0, z = 0;

  n = sscanf("192.168.178.27", "%d.%d.%d.%d", &a, &b, &c, &d);
  check(n == 4 && a == 192 && b == 168 && c == 178 && d == 27, "%d.%d.%d.%d");

  a = b = -1;
  n = sscanf("7,8", "%d,%d", &a, &b);
  check(n == 2 && a == 7 && b == 8, "%d,%d");

  a = b = c = d = -1;
  n = sscanf("(192,168,178,158,235,40)", "(%d,%d,%d,%d", &a, &b, &c, &d);
  check(n == 4 && a == 192 && b == 168 && c == 178 && d == 158, "(%d,%d,%d,%d");

  n = sscanf("20261024114051", "%04hd%02hd%02hd%02hd%02hd%02hd", &y, &mo, &da, &h, &mi, &s);
  check(n == 6 && y == 2026 && mo == 10 && da == 24 && h == 11 && mi == 40 && s == 51,
        "%04hd%02hd%02hd%02hd%02hd%02hd");

  n = sscanf("FF0123:4E71", "%lx:%lx", &x, &z);
  check(n == 2 && x == 0xFF0123UL && z == 0x4E71UL, "%lx:%lx");

  a = b = c = -1;
  n = sscanf("INDEX 01 12:34:56", "INDEX 01 %02d:%02d:%02d", &a, &b, &c);
  check(n == 3 && a == 12 && b == 34 && c == 56, "INDEX 01 %02d:%02d:%02d");

  a = -1;
  n = sscanf("REM LOOP 10", "REM LOOP %d", &a);
  check(n == 1 && a == 10, "REM LOOP %d");

  /* these never depended on the pushed-back character */
  a = b = c = d = -1;
  n = sscanf("1 2 3 4", "%d %d %d %d", &a, &b, &c, &d);
  check(n == 4 && a == 1 && b == 2 && c == 3 && d == 4, "%d %d %d %d");

  a = -1;
  n = sscanf("42", "%d", &a);
  check(n == 1 && a == 42, "%d");

  printf("%d failures\n", failures);
  return failures;
}
