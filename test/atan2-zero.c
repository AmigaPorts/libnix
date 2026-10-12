/* Run against the real Amiga libm, or test/run-atan2-zero.py on a host. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#ifdef LIBNIX_HOST_TEST
extern double libnix_atan2(double, double);
#define TEST_ATAN2 libnix_atan2
#else
#define TEST_ATAN2 atan2
#endif

static double from_bits(uint64_t bits)
{
  union { double f; uint64_t bits; } v;
  v.bits = bits;
  return v.f;
}

static uint64_t to_bits(double f)
{
  union { double f; uint64_t bits; } v;
  v.f = f;
  return v.bits;
}

int main(void)
{
  static const struct { uint64_t y, x, expected; } cases[] = {
    { 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL },
    { 0x8000000000000000ULL, 0x0000000000000000ULL, 0x8000000000000000ULL },
    { 0x0000000000000000ULL, 0x8000000000000000ULL, 0x400921fb54442d18ULL },
    { 0x8000000000000000ULL, 0x8000000000000000ULL, 0xc00921fb54442d18ULL },
    { 0x0000000000000000ULL, 0x3ff0000000000000ULL, 0x0000000000000000ULL },
    { 0x8000000000000000ULL, 0x3ff0000000000000ULL, 0x8000000000000000ULL },
    { 0x0000000000000000ULL, 0xbff0000000000000ULL, 0x400921fb54442d18ULL },
    { 0x8000000000000000ULL, 0xbff0000000000000ULL, 0xc00921fb54442d18ULL },
    { 0x0000000000000000ULL, 0x7ff0000000000000ULL, 0x0000000000000000ULL },
    { 0x8000000000000000ULL, 0x7ff0000000000000ULL, 0x8000000000000000ULL },
    { 0x0000000000000000ULL, 0xfff0000000000000ULL, 0x400921fb54442d18ULL },
    { 0x8000000000000000ULL, 0xfff0000000000000ULL, 0xc00921fb54442d18ULL },
    { 0x3ff0000000000000ULL, 0x0000000000000000ULL, 0x3ff921fb54442d18ULL },
    { 0xbff0000000000000ULL, 0x0000000000000000ULL, 0xbff921fb54442d18ULL },
    { 0x3ff0000000000000ULL, 0x8000000000000000ULL, 0x3ff921fb54442d18ULL },
    { 0xbff0000000000000ULL, 0x8000000000000000ULL, 0xbff921fb54442d18ULL },
    { 0x3ff0000000000000ULL, 0x3ff0000000000000ULL, 0x3fe921fb54442d18ULL },
    { 0x3ff0000000000000ULL, 0xbff0000000000000ULL, 0x4002d97c7f3321d2ULL },
    { 0xbff0000000000000ULL, 0xbff0000000000000ULL, 0xc002d97c7f3321d2ULL },
    { 0xbff0000000000000ULL, 0x3ff0000000000000ULL, 0xbfe921fb54442d18ULL },
  };
  double (*volatile fn)(double, double) = TEST_ATAN2;
  unsigned int i;
  int failures = 0;
  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    uint64_t got = to_bits(fn(from_bits(cases[i].y), from_bits(cases[i].x)));
    if (got != cases[i].expected) {
      printf("case %u: got %016llx, expected %016llx\n", i,
             (unsigned long long)got, (unsigned long long)cases[i].expected);
      ++failures;
    }
  }
  /* A zero y must not hide a NaN x behind the new early return. */
  for (i = 0; i < 4; ++i) {
    uint64_t got = to_bits(fn(from_bits(i & 1 ? 0x8000000000000000ULL : 0),
                                from_bits(i & 2 ? 0xfff8000000000000ULL : 0x7ff8000000000000ULL)));
    if ((got & 0x7fffffffffffffffULL) <= 0x7ff0000000000000ULL) {
      printf("NaN case %u: got %016llx\n", i, (unsigned long long)got);
      ++failures;
    }
  }
  printf("atan2 signed-zero: %s (24 cases)\n", failures ? "FAIL" : "PASS");
  return failures ? 1 : 0;
}
