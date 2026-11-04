/* printf() of subnormal and smallest normal long doubles.  The 68881's
   exponent field 0 is 2^-16383, and normalizing such a value takes its
   binary exponent below the bias.  Prints what fails; the exit code is the
   number of failures.  */

#include <stdio.h>
#include <string.h>

static int failures;
static char buf[128];

static void expect(const char *fmt, long double x, const char *want)
{
  snprintf(buf, sizeof(buf), fmt, x);
  if (strcmp(buf, want))
    {
      printf("FAIL: \"%s\" printed \"%s\", want \"%s\"\n", fmt, buf, want);
      failures++;
    }
}

int main(void)
{
  volatile long double true_min = 0x1p-16446L;
  int n;

  expect("%Le", true_min, "1.822600e-4951");
  expect("%Le", 0x1p-16445L, "3.645200e-4951");
  expect("%Le", 0x1p-16400L, "1.282541e-4937");
  expect("%Le", 0x3p-16420L, "3.669378e-4943");
  expect("%.16Le", true_min, "1.8225997659412373e-4951");
  expect("%Le", 0x1p-16383L, "1.681052e-4932");
  expect("%Le", 0x1p-16382L, "3.362103e-4932");
  expect("%Lg", 0x1p-16400L, "1.28254e-4937");
  expect("%.3Lf", true_min, "0.000");
  expect("%.3Lf", -0x1p-16400L, "-0.000");
  /* and a subnormal double, which takes the same path */
  snprintf(buf, sizeof(buf), "%e", 0x1p-1074);
  if (strcmp(buf, "4.940656e-324"))
    {
      printf("FAIL: \"%%e\" of 0x1p-1074 printed \"%s\"\n", buf);
      failures++;
    }

  n = snprintf(NULL, 0, "%.3Lf", true_min);
  if (n != 5)
    {
      printf("FAIL: snprintf(NULL, 0, \"%%.3Lf\") of 0x1p-16446L returned %d, want 5\n", n);
      failures++;
    }
  return failures;
}
