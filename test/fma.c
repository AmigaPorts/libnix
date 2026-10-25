/* fma() and fmaf(): one rounding of the exact x * y + z (x * y + z as
   written rounds twice), in the current rounding mode where there is one,
   and a result that is a double or a float, not an extended value left in
   fp0.  Prints what fails; the exit code is the number of failures.  */

#include <fenv.h>
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

int main(void)
{
  volatile double one = 1.0, eps = 0x1p-52, big = 0x1p256, tiny = 0x1p-256;
  volatile float onef = 1.0f, epsf = 0x1p-23f;
  volatile double r;
  volatile float rf;

  /* (1 + 2^-52)(1 - 2^-52) = 1 - 2^-104: only the fused sum sees the 2^-104 */
  r = fma(one + eps, one - eps, -one);
  check(r == -0x1p-104, "fma(1+e, 1-e, -1) == -2^-104");
  rf = fmaf(onef + epsf, onef - epsf, -onef);
  check(rf == -0x1p-46f, "fmaf(1+e, 1-e, -1) == -2^-46");

  r = fma(one, one, 0x1p-200);
  check(r == 1.0, "fma(1, 1, 2^-200) == 1 in round to nearest");
  r = fma(big, big, -tiny);
  check(r == 0x1p512, "fma(2^256, 2^256, -2^-256) == 2^512 in round to nearest");

  r = fma(0x1p-1022, 0.5, 0.0);
  check(r == 0x1p-1023, "fma(2^-1022, 0.5, 0) == 2^-1023 (subnormal)");
  r = fma(one, 0.0, -0.0);
  check(r == 0.0 && !signbit(r), "fma(1, 0, -0) == +0");
  r = fma(-one, 0.0, -0.0);
  check(r == 0.0 && signbit(r), "fma(-1, 0, -0) == -0");
  r = fma(one, one, INFINITY);
  check(isinf(r) && r > 0, "fma(1, 1, inf) == inf");
  r = fma(INFINITY, 0.0, one);
  check(isnan(r), "fma(inf, 0, 1) is NaN");
  r = fma(NAN, one, one);
  check(isnan(r), "fma(NaN, 1, 1) is NaN");

  /* the directed rounding modes need a rounding mode to set: the FPU builds */
#if defined(FE_DOWNWARD) && defined(FE_UPWARD)
  if (fesetround(FE_DOWNWARD) == 0)
    {
      /* gcc.dg/pr116891.c, which compares the returned value itself: an
         extended result is not the double below 2^512 */
      check(fma(big, big, -tiny) == nextafter(0x1p512, 0.0),
            "fma(2^256, 2^256, -2^-256) rounded down");
      r = fma(-big, big, tiny);
      check(r == -0x1p512, "fma(-2^256, 2^256, 2^-256) rounded down");
      fesetround(FE_UPWARD);
      r = fma(one, one, 0x1p-200);
      check(r == nextafter(1.0, 2.0), "fma(1, 1, 2^-200) rounded up");
      fesetround(FE_TONEAREST);
    }
#endif

  printf("%d failures\n", failures);
  return failures;
}
