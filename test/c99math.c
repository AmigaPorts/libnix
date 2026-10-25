/* The C99 math functions libnix lacked: lgamma, tgamma, exp2, log2f,
   nexttoward and the long double functions.  The exact ones are checked
   for exact results, on values that need all 64 mantissa bits of a long
   double; the ones computed in double, against a tolerance.  */

#include <errno.h>
#include <float.h>
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

static int close_to(double got, double want)
{
  double d = got - want;
  if (d < 0)
    d = -d;
  return d <= 1e-12 * (want < 0 ? -want : want) + 1e-300;
}

static int closef(float got, float want)
{
  float d = got - want;
  if (d < 0)
    d = -d;
  return d <= 1e-5f * (want < 0 ? -want : want) + 1e-30f;
}

int main(void)
{
  const double sqrtpi = 1.7724538509055160273;
  long double two62 = 0x1p62L, two61 = 0x1p61L;
  int sign, q;

  /* lgamma, lgamma_r, signgam */
  check(lgamma(1.0) == 0.0 && lgamma(2.0) == 0.0, "lgamma(1) and lgamma(2) are 0");
  check(close_to(lgamma(0.5), 0.57236494292470008707), "lgamma(0.5) is log(sqrt(pi))");
  check(close_to(lgamma(10.0), 12.801827480081469611), "lgamma(10) is log(9!)");
  check(close_to(lgamma(-0.5), 1.2655121234846453965) && signgam == -1,
        "lgamma(-0.5) is log(2 sqrt(pi)), signgam -1");
  check(close_to(lgamma_r(0.5, &sign), 0.57236494292470008707) && sign == 1,
        "lgamma_r(0.5) sets the sign to 1");
  check(closef(lgammaf(0.5f), 0.57236494f), "lgammaf(0.5)");
  check(closef(lgammaf_r(-0.5f, &sign), 1.2655121f) && sign == -1, "lgammaf_r(-0.5)");

  /* tgamma */
  check(close_to(tgamma(5.0), 24.0), "tgamma(5) is 24");
  check(close_to(tgamma(0.5), sqrtpi), "tgamma(0.5) is sqrt(pi)");
  check(close_to(tgamma(-0.5), -2 * sqrtpi), "tgamma(-0.5) is -2 sqrt(pi)");
  check(close_to(tgamma(171.0), 7.257415615307998967e306), "tgamma(171) is 170!");
  errno = 0;
  check(isinf(tgamma(0.0)) && tgamma(0.0) > 0 && errno == ERANGE, "tgamma(0) is +inf, ERANGE");
  errno = 0;
  check(isinf(tgamma(-0.0)) && tgamma(-0.0) < 0, "tgamma(-0) is -inf");
  errno = 0;
  check(isnan(tgamma(-1.0)) && errno == EDOM, "tgamma(-1) is NaN, EDOM");
  errno = 0;
  check(isinf(tgamma(172.0)) && errno == ERANGE, "tgamma(172) overflows, ERANGE");
  check(isnan(tgamma(NAN)), "tgamma(NaN) is NaN");
  check(closef(tgammaf(5.0f), 24.0f), "tgammaf(5) is 24");
  errno = 0;
  check(isinf(tgammaf(36.0f)) && errno == ERANGE, "tgammaf(36) overflows, ERANGE");

  /* exp2, exp2f, log2f */
  check(exp2(10.0) == 1024.0 && exp2(-3.0) == 0.125, "exp2 of an integer is exact");
  check(exp2(-1022.0) == DBL_MIN && exp2(1023.0) == 0x1p1023, "exp2 at the ends of the normal range");
  check(close_to(exp2(0.5), 1.4142135623730950488), "exp2(0.5) is sqrt(2)");
  check(isnan(exp2(NAN)), "exp2(NaN) is NaN");
  check(exp2f(3.0f) == 8.0f && closef(exp2f(0.5f), 1.41421356f), "exp2f");
  check(log2f(8.0f) == 3.0f && log2f(0.25f) == -2.0f, "log2f of a power of 2 is exact");
  check(closef(log2f(10.0f), 3.32192809f), "log2f(10)");

  /* nexttoward, nexttowardf: compared in long double */
  check(nexttoward(1.0, 2.0L) == 1.0 + DBL_EPSILON, "nexttoward(1, 2)");
  check(nexttoward(1.0, 1.0L + 0x1p-60L) == 1.0 + DBL_EPSILON,
        "nexttoward(1, 1 + 2^-60) moves, a long double past 1");
  check(nexttoward(1.0, 1.0L) == 1.0, "nexttoward(1, 1) is 1");
  check(nexttowardf(1.0f, 0.0L) == 1.0f - FLT_EPSILON / 2, "nexttowardf(1, 0)");
  check(isnan(nexttoward(NAN, 1.0L)), "nexttoward(NaN, 1) is NaN");

  /* truncl, roundl, rintl and friends, on values below 2^63 that need
     all 64 bits */
  check(truncl(-2.5L) == -2.0L && truncl(2.75L) == 2.0L, "truncl");
  check(truncl(two62 + 0.5L) == two62, "truncl(2^62 + 0.5) is 2^62");
  check(truncl(-0.5L) == 0.0L && signbit(truncl(-0.5L)), "truncl(-0.5) is -0");
  check(truncl(0x1p63L + 2.0L) == 0x1p63L + 2.0L, "truncl of an integer above 2^63");
  check(roundl(2.5L) == 3.0L && roundl(-2.5L) == -3.0L && roundl(2.4L) == 2.0L, "roundl");
  check(roundl(two62 + 0.5L) == two62 + 1.0L, "roundl(2^62 + 0.5) is 2^62 + 1");
  check(roundl(-0.3L) == 0.0L && signbit(roundl(-0.3L)), "roundl(-0.3) is -0");
  check(rintl(2.5L) == 2.0L && rintl(3.5L) == 4.0L && rintl(-2.5L) == -2.0L, "rintl rounds to even");
  check(rintl(two61 + 1.5L) == two61 + 2.0L, "rintl(2^61 + 1.5) is 2^61 + 2");
  check(rintl(-0.4L) == 0.0L && signbit(rintl(-0.4L)), "rintl(-0.4) is -0");
  check(nearbyintl(0.5L) == 0.0L, "nearbyintl(0.5) is 0");
  check(lrintl(2.5L) == 2 && lroundl(2.5L) == 3 && lroundl(-2.5L) == -3, "lrintl, lroundl");
  check(llroundl(two62 + 0.5L) == (1LL << 62) + 1, "llroundl(2^62 + 0.5)");
  check(llrintl(two62 + 0.5L) == 1LL << 62, "llrintl(2^62 + 0.5)");

  /* nextafterl, nexttowardl */
  check(nextafterl(1.0L, 2.0L) - 1.0L == 0x1p-63L, "nextafterl(1, 2) is 1 + 2^-63");
  check(1.0L - nextafterl(1.0L, 0.0L) == 0x1p-64L, "nextafterl(1, 0) is 1 - 2^-64");
  check(nextafterl(nextafterl(1.0L, 0.0L), 2.0L) == 1.0L, "nextafterl back up to 1");
  check(nextafterl(-1.0L, -2.0L) + 1.0L == -0x1p-63L, "nextafterl(-1, -2)");
  check(nextafterl(0.0L, 1.0L) == LDBL_TRUE_MIN, "nextafterl(0, 1) is the smallest denormal");
  check(nextafterl(0.0L, -1.0L) == -LDBL_TRUE_MIN, "nextafterl(0, -1)");
  check(nextafterl(LDBL_TRUE_MIN, -1.0L) == 0.0L, "nextafterl down to 0");
  check(isinf(nextafterl(LDBL_MAX, INFINITY)), "nextafterl(LDBL_MAX, inf) is inf");
  check(nextafterl(INFINITY, 0.0L) == LDBL_MAX, "nextafterl(inf, 0) is LDBL_MAX");
  check(isnan(nextafterl(NAN, 1.0L)), "nextafterl(NaN, 1) is NaN");
  check(nexttowardl(1.0L, 2.0L) == nextafterl(1.0L, 2.0L), "nexttowardl");

  /* scalbnl, ilogbl, logbl */
  check(scalbnl(3.0L, -2) == 0.75L && scalbnl(1.0L, 16000) == 0x1p16000L, "scalbnl");
  check(scalbnl(0x1p16000L, -32000) == 0x1p-16000L, "scalbnl across the whole range");
  check(isinf(scalbnl(1.0L, 20000)) && scalbnl(1.0L, -20000) == 0.0L, "scalbnl overflow, underflow");
  check(scalbnl(1.0L, -16440) == 0x1p-16440L, "scalbnl to a denormal");
  check(scalblnl(1.0L, 100L) == 0x1p100L, "scalblnl");
  check(ilogbl(1024.0L) == 10 && ilogbl(0.75L) == -1 && ilogbl(0x1p-16000L) == -16000, "ilogbl");
  check(ilogbl(LDBL_TRUE_MIN) == -16446, "ilogbl of the smallest denormal");
  check(ilogbl(0.0L) == FP_ILOGB0 && ilogbl(NAN) == FP_ILOGBNAN && ilogbl(INFINITY) == INT_MAX,
        "ilogbl of 0, NaN, inf");
  check(logbl(0.125L) == -3.0L && isinf(logbl(0.0L)) && logbl(0.0L) < 0, "logbl");

  /* copysignl, nanl, fmaxl, fminl, fdiml */
  check(copysignl(3.0L, -0.0L) == -3.0L && copysignl(-3.0L, 1.0L) == 3.0L, "copysignl");
  check(isnan(nanl("")), "nanl");
  check(fmaxl(1.0L, NAN) == 1.0L && fmaxl(NAN, 2.0L) == 2.0L && fmaxl(1.0L, 2.0L) == 2.0L, "fmaxl");
  check(fminl(1.0L, NAN) == 1.0L && fminl(1.0L, 2.0L) == 1.0L, "fminl");
  check(fdiml(5.0L, 3.0L) == 2.0L && fdiml(3.0L, 5.0L) == 0.0L && isnan(fdiml(NAN, 1.0L)), "fdiml");

  /* the ones computed in double */
  check(acoshl(1.0L) == 0.0L && asinhl(0.0L) == 0.0L && atanhl(0.0L) == 0.0L, "acoshl, asinhl, atanhl");
  check(close_to(acoshl(2.0L), 1.3169578969248167086), "acoshl(2)");
  check(exp2l(10.0L) == 1024.0L && close_to(log2l(10.0L), 3.3219280948873623479), "exp2l, log2l");
  check(expm1l(0.0L) == 0.0L && close_to(expm1l(1.0L), 1.7182818284590452354), "expm1l");
  check(log1pl(0.0L) == 0.0L && close_to(log1pl(1.0L), 0.69314718055994530942), "log1pl");
  check(close_to(cbrtl(27.0L), 3.0), "cbrtl(27)");
  check(erfl(0.0L) == 0.0L && erfcl(0.0L) == 1.0L && close_to(erfl(1.0L), 0.84270079294971486934), "erfl, erfcl");
  check(lgammal(1.0L) == 0.0L && close_to(tgammal(5.0L), 24.0), "lgammal, tgammal");
  check(remainderl(10.0L, 3.0L) == 1.0L && remainderl(11.0L, 3.0L) == -1.0L, "remainderl");
  check(remquol(10.0L, 3.0L, &q) == 1.0L && (q & 7) == 3, "remquol");
  check(fmal(2.0L, 3.0L, 4.0L) == 10.0L, "fmal");

  printf("%d failures\n", failures);
  return failures;
}
