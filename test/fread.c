/* fread(): large requests go straight into the caller's memory, the rest
   through the buffer; whatever the mix, the bytes must be the right ones.
   A 100000-byte file of a known pattern on T:, read back in every way that
   crosses the paths: whole, in odd sizes, after getc() and ungetc(), with a
   small setvbuf() buffer, past EOF, and after writing on an update
   stream.  Prints what fails; the exit code is the number of failures.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME "T:libnix-fread-test"
#define SIZE 100000L

static int failures;

static void check(int ok, const char *what)
{
  if (!ok)
    {
      printf("FAIL: %s\n", what);
      failures++;
    }
}

static unsigned char pattern(long i)
{
  return (unsigned char) (i * 7 + (i >> 8));
}

/* buf[0..n) is the file from offset off */
static int matches(const unsigned char *buf, long off, long n)
{
  long i;
  for (i = 0; i < n; i++)
    if (buf[i] != pattern(off + i))
      return 0;
  return 1;
}

int main(void)
{
  unsigned char *buf = malloc(SIZE + 16);
  FILE *f;
  long i, n, pos;
  size_t r;

  if (!buf)
    return 20;
  f = fopen(NAME, "wb");
  check(f != NULL, "fopen for writing");
  if (!f)
    return 20;
  for (i = 0; i < SIZE; i++)
    buf[i] = pattern(i);
  check(fwrite(buf, 1, SIZE, f) == SIZE, "fwrite");
  check(fclose(f) == 0, "fclose after writing");

  /* one request bigger than the buffer, then one past EOF */
  f = fopen(NAME, "rb");
  check(f != NULL, "fopen for reading");
  memset(buf, 0, SIZE);
  r = fread(buf, 1, SIZE + 16, f);
  check(r == SIZE && matches(buf, 0, SIZE), "fread the whole file in one call");
  check(feof(f) && !ferror(f), "feof after the short read");
  check(fread(buf, 1, 1, f) == 0, "fread at EOF reads nothing");
  check(ftell(f) == SIZE, "ftell at EOF");
  fclose(f);

  /* element sizes: a short count is in elements */
  f = fopen(NAME, "rb");
  r = fread(buf, 4096, 40, f);
  check(r == SIZE / 4096 && matches(buf, 0, (long) r * 4096), "fread in 4096-byte elements");
  fclose(f);

  /* sizes that are not multiples of the buffer, from odd offsets:
     the direct read, the tail through the buffer, and the next request
     starting from what the tail left in the buffer */
  f = fopen(NAME, "rb");
  pos = 0;
  for (n = 1; pos < SIZE; n = n * 3 + 1)
    {
      long want = n > SIZE - pos ? SIZE - pos : n;
      memset(buf, 0, want);
      r = fread(buf, 1, want, f);
      check(r == (size_t) want && matches(buf, pos, want), "fread in growing odd sizes");
      pos += want;
    }
  check(fgetc(f) == EOF, "EOF after reading everything in pieces");
  fclose(f);

  /* getc() first, so the buffer is partly used, then a big fread */
  f = fopen(NAME, "rb");
  for (i = 0; i < 10; i++)
    check(fgetc(f) == pattern(i), "getc before fread");
  r = fread(buf, 1, 50000, f);
  check(r == 50000 && matches(buf, 10, 50000), "fread after getc, across the buffer end");
  check(ftell(f) == 50010, "ftell after fread");
  /* ungetc(), then a big fread: the pushed-back byte comes first */
  check(ungetc(pattern(50009), f) == pattern(50009), "ungetc");
  r = fread(buf, 1, 40000, f);
  check(r == 40000 && matches(buf, 50009, 40000), "fread after ungetc");
  /* a second ungetc with the buffer empty, then the rest */
  check(ungetc(pattern(90008), f) == pattern(90008), "ungetc again");
  r = fread(buf, 1, SIZE, f);
  check(r == SIZE - 90008 && matches(buf, 90008, SIZE - 90008), "fread to EOF after ungetc");
  fclose(f);

  /* a tiny buffer: nearly every request is a direct read */
  f = fopen(NAME, "rb");
  check(setvbuf(f, NULL, _IOFBF, 100) == 0, "setvbuf 100 bytes");
  pos = 0;
  while (pos < SIZE)
    {
      long want = 333 > SIZE - pos ? SIZE - pos : 333;
      r = fread(buf, 1, want, f);
      check(r == (size_t) want && matches(buf, pos, want), "fread with a 100-byte buffer");
      pos += want;
    }
  fclose(f);

  /* unbuffered: everything byte by byte, the direct path must stay out */
  f = fopen(NAME, "rb");
  setbuf(f, NULL);
  r = fread(buf, 1, 5000, f);
  check(r == 5000 && matches(buf, 0, 5000), "fread on an unbuffered stream");
  fclose(f);

  /* seek, then read; and write then read on an update stream */
  f = fopen(NAME, "r+b");
  check(fseek(f, 12345, SEEK_SET) == 0, "fseek");
  r = fread(buf, 1, 20000, f);
  check(r == 20000 && matches(buf, 12345, 20000), "fread after fseek");
  check(fseek(f, 0, SEEK_SET) == 0, "fseek to the start");
  check(fwrite("ABCDEFGH", 1, 8, f) == 8, "fwrite on the update stream");
  check(fseek(f, 0, SEEK_SET) == 0, "fseek back after writing");
  r = fread(buf, 1, 30000, f);
  check(r == 30000 && memcmp(buf, "ABCDEFGH", 8) == 0 && matches(buf + 8, 8, 29992),
        "fread sees the write");
  fclose(f);

  remove(NAME);
  free(buf);
  printf("%d failures\n", failures);
  return failures;
}
