/* The 68000's 32-bit multiply and divide helpers, whichever the runtime links
   (built with -m68000, every *, / and % below is a call), checked against a
   shift-and-add multiply and a shift-and-subtract division: divisors of every
   width, the boundaries, and the cases where a quotient estimated from the
   divisor's top 16 bits is one too large.  Then the library's own calls:
   printf's decimal conversion divides, and strtoul multiplies. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long cases;

static unsigned long ref_div(unsigned long n, unsigned long d, unsigned long *r)
{
  unsigned long q = 0, rem = 0;
  int i;

  for (i = 31; i >= 0; --i)
    {
      unsigned long carry = rem >> 31;

      rem = (rem << 1) | ((n >> i) & 1);
      if (carry || rem >= d)
        {
          rem -= d;
          q |= 1UL << i;
        }
    }
  *r = rem;
  return q;
}

static unsigned long ref_mul(unsigned long a, unsigned long b)
{
  unsigned long p = 0;

  for (; b != 0; b >>= 1, a <<= 1)
    if (b & 1)
      p += a;
  return p;
}

static int check(unsigned long n, unsigned long d)
{
  volatile unsigned long vn = n, vd = d;
  volatile long sn = (long) n, sd = (long) d;
  unsigned long r, q = ref_div(n, d, &r);

  ++cases;
  if (vn * vd != ref_mul(n, d))
    {
      printf("FAIL: %#lx * %#lx = %#lx, expected %#lx\n", n, d, vn * vd, ref_mul(n, d));
      return 1;
    }
  if (vn / vd != q || vn % vd != r)
    {
      printf("FAIL: %#lx / %#lx = %#lx rem %#lx, expected %#lx rem %#lx\n",
             n, d, vn / vd, vn % vd, q, r);
      return 1;
    }
  if (sd == -1 && sn == -2147483647L - 1)
    return 0;

  {
    unsigned long an = sn < 0 ? -n : n, ad = sd < 0 ? -d : d, ar;
    unsigned long aq = ref_div(an, ad, &ar);
    long wq = (sn < 0) != (sd < 0) ? (long) -aq : (long) aq;
    long wr = sn < 0 ? (long) -ar : (long) ar;

    if (sn / sd != wq || sn % sd != wr)
      {
        printf("FAIL: %ld / %ld = %ld rem %ld, expected %ld rem %ld\n",
               (long) sn, (long) sd, sn / sd, sn % sd, wq, wr);
        return 1;
      }
  }
  return 0;
}

/* printf("%lu") and strtoul() against digits taken with ref_div(). */
static int check_text(unsigned long n)
{
  char want[16], got[16], *end, *p = want + sizeof want - 1;
  unsigned long v = n, r;

  *p = '\0';
  do
    {
      v = ref_div(v, 10, &r);
      *--p = '0' + r;
    }
  while (v != 0);

  ++cases;
  sprintf(got, "%lu", n);
  if (strcmp(got, p) != 0)
    {
      printf("FAIL: printf(\"%%lu\", %#lx) gave %s, expected %s\n", n, got, p);
      return 1;
    }
  if (strtoul(p, &end, 10) != n || *end != '\0')
    {
      printf("FAIL: strtoul(\"%s\") gave %#lx\n", p, strtoul(p, NULL, 10));
      return 1;
    }
  if (n <= 2147483647UL)
    {
      sprintf(got, "%ld", -(long) n);
      if (got[0] != (n ? '-' : '0') || strcmp(got + (n != 0), p) != 0)
        {
          printf("FAIL: printf(\"%%ld\", -%lu) gave %s\n", n, got);
          return 1;
        }
    }
  return 0;
}

static unsigned long seed = 425;

static unsigned long next(void)
{
  seed = ref_mul(seed, 1103515245UL) + 12345;
  return (seed >> 16) | (seed << 16);
}

int main(void)
{
  static const unsigned short tops[] = { 0x8000, 0x8001, 0xc000, 0xfffe, 0xffff };
  static const unsigned long edges[] = {
    0, 1, 2, 3, 9, 10, 0x7fff, 0x8000, 0xfffe, 0xffff, 0x10000, 0x10001,
    0x1ffff, 0x7fffffff, 0x80000000, 0x80000001, 0xfffeffff, 0xffff0000,
    0xfffffffe, 0xffffffff
  };
  const int nedges = sizeof edges / sizeof edges[0];
  int i, j, k, t;

  for (i = 0; i < nedges; ++i)
    {
      if (check_text(edges[i]))
        return 1;
      for (j = 1; j < nedges; ++j)
        if (check(edges[i], edges[j]))
          return 1;
    }

  for (i = 0; i < 2000; ++i)
    {
      unsigned long d = next() >> (next() & 31);

      if (d == 0)
        d = 1;
      if (check(next(), d) || check(next() >> (next() & 31), d)
          || check_text(next() >> (next() & 31)))
        return 1;
    }

  for (k = 0; k <= 16; ++k)
    for (t = 0; t < 5; ++t)
      {
        unsigned long d = (((unsigned long) tops[t] << 16) | 0xffff) >> k, r, qmax;

        if (d < 0x10000)
          continue;
        qmax = ref_div(0xffffffffUL, d, &r);
        {
          unsigned long qs[] = { 1, 2, qmax, qmax - 1, (qmax >> 1) + 1 };

          for (j = 0; j < 5; ++j)
            {
              unsigned long m = ref_mul(qs[j], d);

              if (check(m, d) || check(m - 1, d) || check(m + d - 1, d))
                return 1;
            }
        }
      }
  return 0;
}
