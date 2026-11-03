/* wmemset() of every length up to 40, at two alignments: the value lands in
   exactly the n elements asked for, and the guards around them stay.  From
   8 up it stores eight at a time with movem.  Prints what fails; the exit
   code is the number of failures.  */

#include <stdio.h>
#include <wchar.h>

static int failures;

int main(void)
{
  static wchar_t buf[48];
  int n, off, i;

  for (off = 0; off < 2; off++)
    for (n = 0; n <= 40; n++)
      {
        wchar_t *p = buf + 1 + off;

        for (i = 0; i < 48; i++)
          buf[i] = 0x11;
        if (wmemset(p, 0x12345678, n) != p)
          {
            printf("FAIL: n=%d off=%d: wrong return value\n", n, off);
            failures++;
          }
        for (i = 0; i < 48; i++)
          {
            int in = buf + i >= p && buf + i < p + n;

            if (buf[i] != (in ? 0x12345678 : 0x11))
              {
                printf("FAIL: n=%d off=%d: buf[%d] = %lx\n", n, off, i, (unsigned long) buf[i]);
                failures++;
                break;
              }
          }
      }
  return failures;
}
