/* Run against the real Amiga libm, or test/run-atan2-zero.py on a host. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#ifdef LIBNIX_HOST_TEST
extern float libnix_atan2f(float, float);
#define TEST_ATAN2F libnix_atan2f
#else
#define TEST_ATAN2F atan2f
#endif

static float from_bits(uint32_t bits)
{
  union { float f; uint32_t bits; } v;
  v.bits = bits;
  return v.f;
}

static uint32_t to_bits(float f)
{
  union { float f; uint32_t bits; } v;
  v.f = f;
  return v.bits;
}

int main(void)
{
  static const struct { uint32_t y, x, expected; } cases[] = {
    { 0x00000000, 0x00000000, 0x00000000 },
    { 0x80000000, 0x00000000, 0x80000000 },
    { 0x00000000, 0x80000000, 0x40490fdb },
    { 0x80000000, 0x80000000, 0xc0490fdb },
    { 0x00000000, 0x3f800000, 0x00000000 },
    { 0x80000000, 0x3f800000, 0x80000000 },
    { 0x00000000, 0xbf800000, 0x40490fdb },
    { 0x80000000, 0xbf800000, 0xc0490fdb },
    { 0x00000000, 0x7f800000, 0x00000000 },
    { 0x80000000, 0x7f800000, 0x80000000 },
    { 0x00000000, 0xff800000, 0x40490fdb },
    { 0x80000000, 0xff800000, 0xc0490fdb },
    { 0x3f800000, 0x00000000, 0x3fc90fdb },
    { 0xbf800000, 0x00000000, 0xbfc90fdb },
    { 0x3f800000, 0x80000000, 0x3fc90fdb },
    { 0xbf800000, 0x80000000, 0xbfc90fdb },
    { 0x3f800000, 0x3f800000, 0x3f490fdb },
    { 0x3f800000, 0xbf800000, 0x4016cbe4 },
    { 0xbf800000, 0xbf800000, 0xc016cbe4 },
    { 0xbf800000, 0x3f800000, 0xbf490fdb },
  };
  float (*volatile fn)(float, float) = TEST_ATAN2F;
  unsigned int i;
  int failures = 0;
  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    uint32_t got = to_bits(fn(from_bits(cases[i].y), from_bits(cases[i].x)));
    if (got != cases[i].expected) {
      printf("case %u: got %08lx, expected %08lx\n", i,
             (unsigned long)got, (unsigned long)cases[i].expected);
      ++failures;
    }
  }
  /* A zero y must not hide a NaN x behind the new early return. */
  for (i = 0; i < 4; ++i) {
    uint32_t got = to_bits(fn(from_bits(i & 1 ? 0x80000000 : 0),
                                from_bits(i & 2 ? 0xffc00000 : 0x7fc00000)));
    if ((got & 0x7fffffffU) <= 0x7f800000U) {
      printf("NaN case %u: got %08lx\n", i, (unsigned long)got);
      ++failures;
    }
  }
  printf("atan2f signed-zero: %s (24 cases)\n", failures ? "FAIL" : "PASS");
  return failures ? 1 : 0;
}
