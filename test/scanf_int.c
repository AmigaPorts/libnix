/* scanf %i and %x prefixes, narrow and wide, with the results glibc gives:
   %i takes its base from a 0 or 0x prefix, "0x" without a hex digit is a
   matching failure, a field width counts the prefix, and the wide functions,
   whose ungetwc() keeps one character, read the same as the narrow ones.
   Prints what fails; the exit code is the number of failures.  */

#include <stdio.h>
#include <wchar.h>

static int failures;

static const struct {
  const char *fmt, *in;
  int r, v, n;
} cases[] = {
  { "%i%n", "0x12", 1, 18, 4 },
  { "%3i%n", "0x12", 1, 1, 3 },
  { "%x%n", "0x12", 1, 18, 4 },
  { "%i%n", "0xg", 0, -1, -1 },
  { "%3i%n", "0xg", 0, -1, -1 },
  { "%x%n", "0xg", 0, -1, -1 },
  { "%i%n", "ff", 0, -1, -1 },
  { "%x%n", "ff", 1, 255, 2 },
  { "%i%n", "0", 1, 0, 1 },
  { "%i%n", "012", 1, 10, 3 },
  { "%x%n", "012", 1, 18, 3 },
  { "%i%n", "0x", 0, -1, -1 },
  { "%x%n", "0x", 0, -1, -1 },
  { "%i%n", "-0x1f", 1, -31, 5 },
  { "%3i%n", "-0x1f", 0, -1, -1 },
  { "%x%n", "-0x1f", 1, -31, 5 },
  { "%i%n", "08", 1, 0, 1 },
  { "%x%n", "08", 1, 8, 2 },
  { "%3i%n", "0X1A", 1, 1, 3 },
};

static void widen(wchar_t *w, const char *s)
{
  while ((*w++ = (unsigned char) *s++))
    ;
}

int main(void)
{
  wchar_t wfmt[16], win[16];
  int v, w, n, r;
  unsigned i;

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
      v = n = -1;
      r = sscanf(cases[i].in, cases[i].fmt, &v, &n);
      if (r != cases[i].r || v != cases[i].v || n != cases[i].n)
        {
          printf("FAIL: sscanf(\"%s\", \"%s\") = %d, v %d, n %d; want %d, %d, %d\n", cases[i].in, cases[i].fmt,
                 r, v, n, cases[i].r, cases[i].v, cases[i].n);
          failures++;
        }
      widen(wfmt, cases[i].fmt);
      widen(win, cases[i].in);
      v = n = -1;
      r = swscanf(win, wfmt, &v, &n);
      if (r != cases[i].r || v != cases[i].v || n != cases[i].n)
        {
          printf("FAIL: swscanf(L\"%s\", L\"%s\") = %d, v %d, n %d; want %d, %d, %d\n", cases[i].in,
                 cases[i].fmt, r, v, n, cases[i].r, cases[i].v, cases[i].n);
          failures++;
        }
    }

  /* the next conversion starts where the width stopped */
  v = w = -1;
  r = swscanf(L"0x12 7", L"%3i%d", &v, &w);
  if (r != 2 || v != 1 || w != 2)
    {
      printf("FAIL: swscanf(L\"0x12 7\", L\"%%3i%%d\") = %d, %d, %d; want 2, 1, 2\n", r, v, w);
      failures++;
    }
  return failures;
}
