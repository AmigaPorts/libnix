/* llrint() from 2^52 up, where the hidden bit went in at bit 16 instead of
   bit 20 and the low word lost its top bits: llrint(0x1p52) was 2^48.  And
   with an FPU, lrint() and the rest below 0.5 in a directed rounding mode,
   where they returned 0.  Prints what fails; the exit code is the number of
   failures.  */

#include <fenv.h>
#include <math.h>
#include <stdio.h>

static int failures;

static void ll(double x, long long want)
{
  volatile double v = x;
  long long r = llrint(v);

  if (r != want)
    {
      printf("FAIL: llrint(%.17g) == %lld, not %lld\n", x, r, want);
      failures++;
    }
}

/* lrint(), llrint(), lrintf() and llrintf() of x all give want.  */
static void all(double x, long want, const char *mode)
{
  volatile double v = x;
  volatile float f = (float)x;
  long r = lrint(v), rf = lrintf(f);
  long long rl = llrint(v), rlf = llrintf(f);

  if (r != want || rl != want || rf != want || rlf != want)
    {
      printf("FAIL: %s: x = %g, want %ld: lrint %ld, llrint %lld, lrintf %ld, llrintf %lld\n",
             mode, x, want, r, rl, rf, rlf);
      failures++;
    }
}

int main(void)
{
  ll(2.5, 2);
  ll(-3.5, -4);
  ll(0x1p52 - 0.5, 1LL << 52);
  ll(0x1p52, 1LL << 52);
  ll(0x1p52 + 1, (1LL << 52) + 1);
  ll(0x1p53, 1LL << 53);
  ll(0x1p53 + 0x1p32, (1LL << 53) + (1LL << 32));
  ll(-0x1p53 - 0x1p32 - 2, -(1LL << 53) - (1LL << 32) - 2);
  ll(0x1.fffffffffffffp+61, 0x3ffffffffffffe00LL);
  ll(0x1.fffffffffffffp+62, 0x7ffffffffffffc00LL);
  ll(-0x1p63, -0x7fffffffffffffffLL - 1);

  all(0.2, 0, "to nearest");
  all(-0.2, 0, "to nearest");
  all(0.7, 1, "to nearest");
  all(-0.7, -1, "to nearest");
  all(1e-30, 0, "to nearest");

  /* the directed rounding modes need a rounding mode to set: the FPU builds */
#if defined(FE_UPWARD) && defined(FE_DOWNWARD) && defined(FE_TOWARDZERO)
  if (fesetround(FE_UPWARD) == 0)
    {
      all(0.2, 1, "upward");
      all(1e-30, 1, "upward");
      all(-0.2, 0, "upward");
      all(-0.7, 0, "upward");
      all(2.5, 3, "upward");
    }
  if (fesetround(FE_DOWNWARD) == 0)
    {
      all(0.2, 0, "downward");
      all(-0.2, -1, "downward");
      all(-1e-30, -1, "downward");
      all(0.7, 0, "downward");
      all(-2.5, -3, "downward");
    }
  if (fesetround(FE_TOWARDZERO) == 0)
    {
      all(0.7, 0, "toward zero");
      all(-0.7, 0, "toward zero");
      all(2.5, 2, "toward zero");
    }
  fesetround(FE_TONEAREST);
#endif

  printf("%d failures\n", failures);
  return failures;
}
