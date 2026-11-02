/* lround() at the end of a 32-bit long's range, where rounding up can
   carry out of it: 2147483647.5 came out as LONG_MIN.  A result that does
   not fit is LONG_MAX or LONG_MIN by its sign, as the conversion gives for
   anything larger, and with an FPU it raises FE_INVALID.  Prints what
   fails; the exit code is the number of failures.  */

#include <fenv.h>
#include <limits.h>
#include <math.h>
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

#ifdef FE_INVALID
/* Out of line, so that the status register is read after each call.  */
static __attribute__((noipa)) void clear_invalid(void)
{
  feclearexcept(FE_INVALID);
}

static __attribute__((noipa)) int invalid(void)
{
  return fetestexcept(FE_INVALID) != 0;
}
#else
static void clear_invalid(void)
{
}

static int invalid(void)
{
  return -1;
}
#endif

/* lround(x) is want, and raises FE_INVALID if and only if it should.  */
static void lr(double x, long want, int out_of_range, const char *what)
{
  volatile double v = x;
  long r;
  int inv;

  clear_invalid();
  r = lround(v);
  inv = invalid();
  check(r == want, what);
  check(inv < 0 || inv == out_of_range, what);
}

int main(void)
{
  lr(0.5, 1, 0, "lround(0.5) == 1");
  lr(-2.5, -3, 0, "lround(-2.5) == -3");
  lr(1073741823.5, 1073741824, 0, "lround(1073741823.5) == 1073741824");
  lr(1073741824.5, 1073741825, 0, "lround(1073741824.5) == 1073741825");
  lr(-1073741824.5, -1073741825, 0, "lround(-1073741824.5) == -1073741825");
  lr(2147483646.5, LONG_MAX, 0, "lround(2147483646.5) == LONG_MAX");
  lr(2147483647.4, LONG_MAX, 0, "lround(2147483647.4) == LONG_MAX");
  lr(-2147483647.5, LONG_MIN, 0, "lround(-2147483647.5) == LONG_MIN");
  lr(-2147483648.4, LONG_MIN, 0, "lround(-2147483648.4) == LONG_MIN");

  /* out of range */
  lr(2147483647.5, LONG_MAX, 1, "lround(2147483647.5) is LONG_MAX, invalid");
  lr(2147483648.0, LONG_MAX, 1, "lround(2147483648) is LONG_MAX, invalid");
  lr(-2147483648.5, LONG_MIN, 1, "lround(-2147483648.5) is LONG_MIN, invalid");
  lr(-2147483649.0, LONG_MIN, 1, "lround(-2147483649) is LONG_MIN, invalid");
  lr(-1e10, LONG_MIN, 1, "lround(-1e10) is LONG_MIN, invalid");

  /* halves round away from zero in any rounding mode: the FPU builds */
#if defined(FE_UPWARD) && defined(FE_DOWNWARD) && defined(FE_TOWARDZERO)
  {
    static const int modes[] = { FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO };
    unsigned i;

    for (i = 0; i < sizeof modes / sizeof *modes; i++)
      if (fesetround(modes[i]) == 0)
        {
          lr(1073741824.5, 1073741825, 0, "lround(1073741824.5), directed mode");
          lr(-1073741824.5, -1073741825, 0, "lround(-1073741824.5), directed mode");
          lr(1073741824.25, 1073741824, 0, "lround(1073741824.25), directed mode");
          lr(-2147483647.75, LONG_MIN, 0, "lround(-2147483647.75), directed mode");
          fesetround(FE_TONEAREST);
        }
  }
#endif

  printf("%d failures\n", failures);
  return failures;
}
