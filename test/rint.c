/* llrint() from 2^52 up, where the hidden bit went in at bit 16 instead of
   bit 20 and the low word lost its top bits: llrint(0x1p52) was 2^48.
   Prints what fails; the exit code is the number of failures.  */

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

  printf("%d failures\n", failures);
  return failures;
}
