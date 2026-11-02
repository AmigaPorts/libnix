/* What the math functions give back for what they can't answer.  The values are
   checked on every library, errno only without an FPU; with one, the flags in
   fenv.h are the report.  */

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

static int failures, checks;

static volatile double zero = 0.0, one = 1.0, two = 2.0, big = 1e308;
static volatile float fzero = 0.0f, fone = 1.0f, ftwo = 2.0f, fbig = 1e38f;
static volatile long double lzero = 0.0L, lone = 1.0L;

/* The bits, so that this is not at the mercy of what it is testing.  */
static int is_nan(double x)
{
  union { double d; unsigned long w[2]; } u;
  u.d = x;
  return (u.w[0] & 0x7fffffffUL) > 0x7ff00000UL ||
	 ((u.w[0] & 0x7fffffffUL) == 0x7ff00000UL && u.w[1] != 0);
}

static int is_inf(double x, int sign)
{
  union { double d; unsigned long w[2]; } u;
  u.d = x;
  return u.w[0] == (sign < 0 ? 0xfff00000UL : 0x7ff00000UL) && u.w[1] == 0;
}

static int is_negative(double x)
{
  union { double d; unsigned long w[2]; } u;
  u.d = x;
  return u.w[0] >> 31;
}

static void result(const char *what, int ok, int want_errno)
{
  checks++;
  if (!ok)
    {
      printf("FAIL: %s: wrong value\n", what);
      failures++;
    }
#ifndef __HAVE_68881__
  if (errno != want_errno)
    {
      printf("FAIL: %s: errno %d, want %d\n", what, errno, want_errno);
      failures++;
    }
#else
  (void)want_errno;
#endif
}

/* The argument is evaluated with errno clear.  */
#define CHECK(expr, ok_expr, want) \
  do { \
    double r; \
    errno = 0; \
    r = (double)(expr); \
    (void)r; \
    result(#expr, ok_expr, want); \
  } while (0)

#define IS_NAN(expr, want)     CHECK(expr, is_nan(r), want)
#define IS_INF(expr, sign, want) CHECK(expr, is_inf(r, sign), want)
#define IS(expr, value, want)  CHECK(expr, r == (value), want)
#define IS_NEG0(expr, want)    CHECK(expr, r == 0 && is_negative(r), want)

#define NO_ERROR(expr) \
  do { \
    errno = 0; \
    (void)(expr); \
    result(#expr, 1, 0); \
  } while (0)

#define ROUNDED(expr) \
  do { \
    volatile long long n; \
    errno = 0; \
    n = (expr); \
    (void)n; \
    result(#expr, 1, EDOM); \
  } while (0)

int main(void)
{
  volatile double inf = one / zero, nan = zero / zero;
  volatile float finf = fone / fzero, fnan = fzero / fzero;
  volatile double max = DBL_MAX;
  int q;

  /* Domain errors: a NaN, whatever the library made of it.  */
  IS_NAN(log(-one), EDOM);
  IS_NAN(log10(-one), EDOM);
  IS_NAN(log2(-one), EDOM);
  IS_NAN(log1p(-two), EDOM);
  IS_NAN(sqrt(-one), EDOM);
  IS_NAN(asin(two), EDOM);
  IS_NAN(acos(two), EDOM);
  IS_NAN(acosh(zero), EDOM);
  IS_NAN(atanh(two), EDOM);
  IS_NAN(sin(inf), EDOM);
  IS_NAN(cos(inf), EDOM);
  IS_NAN(tan(inf), EDOM);
  IS_NAN(pow(-one, 0.5), EDOM);
  IS_NAN(fmod(one, zero), EDOM);
  IS_NAN(fmod(inf, one), EDOM);
  IS_NAN(remainder(one, zero), EDOM);
  IS_NAN(remquo(one, zero, &q), EDOM);
  IS_NAN(fma(inf, zero, one), EDOM);
  IS_NAN(logf(-fone), EDOM);
  IS_NAN(log10f(-fone), EDOM);
  IS_NAN(sqrtf(-fone), EDOM);
  IS_NAN(asinf(ftwo), EDOM);
  IS_NAN(acosf(ftwo), EDOM);
  IS_NAN(sinf(finf), EDOM);
  IS_NAN(cosf(finf), EDOM);
  IS_NAN(tanf(finf), EDOM);
  IS_NAN(powf(-fone, 0.5f), EDOM);
  IS_NAN(fmodf(fone, fzero), EDOM);
  IS_NAN(remainderf(fone, fzero), EDOM);
  IS_NAN(fmaf(finf, fzero, fone), EDOM);
  IS_NAN(logl(-lone), EDOM);
  IS_NAN(sqrtl(-lone), EDOM);

  /* Poles and overflows: an infinity, not DBL_MAX and not the logarithm of it.  */
  IS_INF(log(zero), -1, ERANGE);
  IS_INF(log10(zero), -1, ERANGE);
  IS_INF(log2(zero), -1, ERANGE);
  IS_INF(log1p(-one), -1, ERANGE);
  IS_INF(logb(zero), -1, ERANGE);
  IS_INF(atanh(one), 1, ERANGE);
  IS_INF(atanh(-one), -1, ERANGE);
  IS_INF(pow(zero, -one), 1, ERANGE);
  IS_INF(pow(-zero, -one), -1, ERANGE);
  IS_INF(lgamma(zero), 1, ERANGE);
  IS_INF(exp(1000 * one), 1, ERANGE);
  IS_INF(expm1(1000 * one), 1, ERANGE);
  IS_INF(exp2(5000 * one), 1, ERANGE);
  IS_INF(sinh(1000 * one), 1, ERANGE);
  IS_INF(sinh(-1000 * one), -1, ERANGE);
  IS_INF(cosh(1000 * one), 1, ERANGE);
  IS_INF(pow(two, 2000 * one), 1, ERANGE);
  IS_INF(pow(-two, 2001 * one), -1, ERANGE);
  IS_INF(hypot(1.5 * big, 1.5 * big), 1, ERANGE);
  IS_INF(scalbn(one, 5000), 1, ERANGE);
  IS_INF(scalbln(-one, 5000L), -1, ERANGE);
  IS_INF(fdim(big, -big), 1, ERANGE);
  IS_INF(nextafter(max, inf), 1, ERANGE);
  IS_INF(fma(big, big, zero), 1, ERANGE);
  IS_INF(logf(fzero), -1, ERANGE);
  IS_INF(expf(1000 * fone), 1, ERANGE);
  IS_INF(sinhf(1000 * fone), 1, ERANGE);
  IS_INF(coshf(100 * fone), 1, ERANGE);
  IS_INF(powf(fzero, -fone), 1, ERANGE);
  IS_INF(powf(ftwo, 200 * fone), 1, ERANGE);
  IS_INF(hypotf(3 * fbig, 3 * fbig), 1, ERANGE);
  IS_INF(exp2f(200 * fone), 1, ERANGE);

  /* Underflow.  */
  IS(exp(-1000 * one), 0, ERANGE);
  IS(expf(-1000 * fone), 0, ERANGE);
  IS(pow(10 * one, -1000 * one), 0, ERANGE);

  /* NaN in, NaN out, and no error.  */
  IS_NAN(sin(nan), 0);
  IS_NAN(cos(nan), 0);
  IS_NAN(exp(nan), 0);
  IS_NAN(log(nan), 0);
  IS_NAN(sqrt(nan), 0);
  IS_NAN(sinh(nan), 0);
  IS_NAN(cosh(nan), 0);
  IS_NAN(tanh(nan), 0);
  IS_NAN(asin(nan), 0);
  IS_NAN(pow(nan, one), 0);
  IS_NAN(pow(one + one, nan), 0);
  IS_NAN(fmod(nan, one), 0);
  IS_NAN(fma(nan, one, inf), 0);
  IS_NAN(atanf(fnan), 0);
  IS_NAN(tanhf(fnan), 0);
  IS_NAN(expf(fnan), 0);
  IS_NAN(atan2f(fnan, fone), 0);

  /* What is defined for an infinity, and for the things that look like errors
     but aren't.  */
  IS_INF(log(inf), 1, 0);
  IS_INF(sqrt(inf), 1, 0);
  IS_INF(exp(inf), 1, 0);
  IS(exp(-inf), 0, 0);
  IS(expm1(-inf), -1, 0);
  IS_INF(sinh(inf), 1, 0);
  IS_INF(sinh(-inf), -1, 0);
  IS_INF(cosh(-inf), 1, 0);
  IS_INF(hypot(inf, nan), 1, 0);
  IS_INF(hypotf(fnan, finf), 1, 0);
  IS(pow(nan, zero), 1, 0);
  IS(pow(one, nan), 1, 0);
  IS(pow(-one, inf), 1, 0);
  IS(pow(two, -inf), 0, 0);
  IS_INF(pow(two, inf), 1, 0);
  IS(pow(0.5, inf), 0, 0);
  IS_INF(pow(-inf, 3 * one), -1, 0);
  IS(pow(-8 * one, 3 * one), -512, 0);
  IS(pow(-two, 2 * one), 4, 0);
  IS(powf(-2 * ftwo, 3 * fone), -64, 0);
  IS(fmod(one, inf), 1, 0);
  IS(fdim(-inf, one), 0, 0);
  IS(fdim(inf, inf), 0, 0);
  IS_INF(fdim(one, -inf), 1, 0);
  IS(hypot(3 * one, 4 * one), 5, 0);
  CHECK(hypot(3e300 * one, 4e300 * one), r > 4.99999999e300 && r < 5.00000001e300, 0);
  IS(hypot(zero, -3 * one), 3, 0);
  IS(hypotf(3 * fone, 4 * fone), 5, 0);
  IS(pow(max, one), DBL_MAX, 0);
  IS_NEG0(sqrt(-zero), 0);
  IS(ceil(zero), 0, 0);
  IS_NEG0(ceil(-zero), 0);
  IS(floor(zero), 0, 0);
  IS_NEG0(floor(-zero), 0);
  IS(ceilf(fzero), 0, 0);
  IS_NEG0(floorf(-fzero), 0);

  /* sinh() and cosh() past the end of exp().  */
  CHECK(sinh(710 * one), r > 1.1e308 && r < 1.2e308, 0);
  CHECK(cosh(-710 * one), r > 1.1e308 && r < 1.2e308, 0);

  /* Either side of the end of the range.  The libraries stop at DBL_MAX,
     so pow(2,1024) and pow(DBL_MAX,1) look the same to them.  */
  IS_INF(pow(two, 1024 * one), 1, ERANGE);
  IS_INF(pow(0.5 * one, -1024 * one), 1, ERANGE);
  IS_INF(pow(16 * one, 256 * one), 1, ERANGE);
  IS_INF(exp2(1024 * one), 1, ERANGE);
  IS(pow(two, 1023 * one), 0x1p1023, 0);
  IS(pow(-max, one), -DBL_MAX, 0);
  IS_INF(pow(10 * one, max), 1, ERANGE);
  IS(pow(10 * one, -max), 0, ERANGE);
  IS(pow(-one, max), 1, 0);
  IS_INF(powf(ftwo, 128 * fone), 1, ERANGE);
  IS_INF(powf(4 * fone, 64 * fone), 1, ERANGE);
  IS(powf(ftwo, 127 * fone), 0x1p127f, 0);
  IS(powf(FLT_MAX * fone, fone), FLT_MAX, 0);
  IS_INF(powf(10 * fone, FLT_MAX * fone), 1, ERANGE);
  IS(powf(10 * fone, -FLT_MAX * fone), 0, ERANGE);
  CHECK(lgamma(2.5599833278516383e305 * one), r > 1.79e308 && !is_inf(r, 1), 0);
  IS_INF(lgamma(2.5599833278516387e305 * one), 1, ERANGE);
  CHECK(lgammaf(4.085003108497519e36f * fone), r > 3.4e38 && !is_inf(r, 1), 0);
#ifndef __HAVE_68881__
  IS_INF(lgammaf(4.085003425410169e36f * fone), 1, ERANGE);
#else
  /* x * (logf(x) - 1) on the FPU comes out an ulp short of overflowing.  */
  CHECK(lgammaf(4.085003425410169e36f * fone), r >= FLT_MAX, 0);
#endif
  CHECK(coshf(89.41598510742188f * fone), r > 3.4e38 && !is_inf(r, 1), 0);
  CHECK(sinhf(-89.41598510742188f * fone), r < -3.4e38 && !is_inf(r, -1), 0);
  IS_INF(coshf(-89.4159927368164f * fone), 1, ERANGE);
  IS_INF(sinhf(89.4159927368164f * fone), 1, ERANGE);

  /* The integer functions: C leaves this open, and glibc says EDOM.  */
#ifndef __HAVE_68881__
    {
      ROUNDED(ilogb(zero));
      ROUNDED(ilogb(inf));
      ROUNDED(ilogb(nan));
      ROUNDED(ilogbf(fzero));
      ROUNDED(ilogbl(lzero));
      ROUNDED(lrint(big));
      ROUNDED(lrint(nan));
      ROUNDED(lround(inf));
      ROUNDED(lround(2147483647.5 * one));
      ROUNDED(llrint(big));
      ROUNDED(llround(-big));
      ROUNDED(lrintf(fbig));
      ROUNDED(lroundf(fnan));
      ROUNDED(llrintf(fbig));
      ROUNDED(llroundf(finf));
      ROUNDED(lrintl(1e300L * lone));
      ROUNDED(lroundl(1e300L * lone));
      ROUNDED(llrintl(1e30L * lone));
      ROUNDED(llroundl(1e30L * lone));
      NO_ERROR(lrint(-2147483648.0 * one));
      NO_ERROR(lround(2147483647.0 * one));
      NO_ERROR(llround(-9223372036854775808.0 * one));
      NO_ERROR(lroundl(-2147483648.0L * lone));
      NO_ERROR(ilogb(one));
    }
#endif

  printf("%d checks, %d failures\n", checks, failures);
  return failures != 0;
}
