/* memset() of every length up to 150, at four alignments: the byte lands in
   exactly the n bytes asked for, and the guards around them stay.  Above 18
   bytes it aligns and stores 32 at a time with movem.  Prints what fails;
   the exit code is the number of failures.  */

#include <stdio.h>
#include <string.h>

static int failures;

int main(void)
{
  static unsigned char buf[168];
  int n, off, i;

  for (off = 0; off < 4; off++)
    for (n = 0; n <= 150; n++)
      {
        unsigned char *p = buf + 4 + off;

        for (i = 0; i < (int) sizeof(buf); i++)
          buf[i] = 0x11;
        /* the int's high bits must not matter */
        if (memset(p, 0x1a5, n) != p)
          {
            printf("FAIL: n=%d off=%d: wrong return value\n", n, off);
            failures++;
          }
        for (i = 0; i < (int) sizeof(buf); i++)
          {
            int in = buf + i >= p && buf + i < p + n;

            if (buf[i] != (in ? 0xa5 : 0x11))
              {
                printf("FAIL: n=%d off=%d: buf[%d] = %x\n", n, off, i, buf[i]);
                failures++;
                break;
              }
          }
      }
  return failures;
}
