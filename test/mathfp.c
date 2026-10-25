/* The fdlibm fmodf, frexp, ldexp, fabs and fabsf from sources/math/mathfp,
   which used to share a file name with weaker versions in trans/ and
   extra/: whichever find listed last went into libm, and fmod.  fmodf and
   fmod are exact, frexp returns an infinity instead of looping forever,
   and ldexp overflows and underflows correctly.  */

#include <float.h>
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
  volatile double d;
  int e;

  check(fmodf(1e10f, 3.0f) == 1.0f, "fmodf(1e10, 3) is exactly 1");
  check(fmodf(-7.0f, 3.0f) == -1.0f, "fmodf(-7, 3) is -1");
  check(fmod(1e20, 3.0) == 1.0, "fmod(1e20, 3) is exactly 1");
  check(fmod(-7.0, 3.0) == -1.0 && fmod(5.5, 2.0) == 1.5, "fmod(-7, 3), fmod(5.5, 2)");

  e = 1;
  check(isinf(frexp(INFINITY, &e)), "frexp(inf) returns inf");
  check(frexp(0x1p-1070, &e) == 0.5 && e == -1069, "frexp of a denormal");
  check(frexp(-12.0, &e) == -0.75 && e == 4, "frexp(-12) is -0.75 * 2^4");

  check(ldexp(0.75, 2) == 3.0, "ldexp(0.75, 2) is 3");
  /* stored as doubles: with a 68881, libm (built with -ffast-math)
     returns the extended-precision result in fp0, beyond double's range */
  d = ldexp(1.0, 1024);
  check(isinf(d), "ldexp(1, 1024) overflows to inf");
  d = ldexp(1.0, -1074);
  check(d == DBL_TRUE_MIN, "ldexp(1, -1074) is the smallest denormal");
  d = ldexp(1.0, -1080);
  check(d == 0.0, "ldexp(1, -1080) underflows to 0");

  check(fabs(-0.0) == 0.0 && !signbit(fabs(-0.0)), "fabs(-0) is +0");
  check(fabs(-2.5) == 2.5 && fabsf(-2.5f) == 2.5f, "fabs, fabsf");

  printf("%d failures\n", failures);
  return failures;
}
