/* Reading floating point rounds once, to nearest even: digits past the
   96-bit accumulator only count as a sticky bit, and zeros not even that,
   and a float is rounded from the digits, not through double.  Prints what
   fails; the exit code is the number of failures.  */

#include <stdio.h>
#include <stdlib.h>

static int failures;

static void check_d(const char *s, double want)
{
  double d = strtod(s, NULL);

  if (d != want)
    {
      printf("FAIL: strtod(\"%s\") = %a, want %a\n", s, d, want);
      failures++;
    }
}

static void check_f(const char *s, float want)
{
  float f = strtof(s, NULL), g = -1;

  if (f != want)
    {
      printf("FAIL: strtof(\"%s\") = %a, want %a\n", s, (double) f, (double) want);
      failures++;
    }
  if (sscanf(s, "%f", &g) != 1 || g != want)
    {
      printf("FAIL: sscanf(\"%s\", \"%%f\") = %a, want %a\n", s, (double) g, (double) want);
      failures++;
    }
}

int main(void)
{
  long double ld = strtold("0x1.000000000000000000000008p0", NULL);

  if (ld != 1.0L)
    {
      printf("FAIL: strtold(\"0x1.000000000000000000000008p0\") = %La, want 0x1p+0\n", ld);
      failures++;
    }

  /* exact ties, with and without zeros past the accumulator */
  check_d("0x1.00000000000008p0", 1.0);
  check_d("0x1.0000000000000800000000000000000000p0", 1.0);
  check_d("0x1.00000000000018p0", 0x1.0000000000002p0);
  /* just above the tie, by a digit past the accumulator */
  check_d("0x1.00000000000008000000000000000000001p0", 0x1.0000000000001p0);

  check_f("0x1.00000100000001p0", 0x1.000002p0);
  check_f("0x1.000001p0", 1.0f);
  check_f("0x1.000003p0", 0x1.000004p0);
  check_f("1.000000059604644775390625", 1.0f);
  check_f("1.0000000596046447753907", 0x1.000002p0);
  check_f("0x1p-149", 0x1p-149f);
  check_f("0x1p-150", 0.0f);
  check_f("0x1.000002p-150", 0x1p-149f);
  check_f("0x1.8p-149", 0x1p-148f);
  check_f("1e-45", 0x1p-149f);
  check_f("3.4028235e38", 0x1.fffffep127f);
  check_f("0.1", 0x1.99999ap-4f);
  return failures;
}
