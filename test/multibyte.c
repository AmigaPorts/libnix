/* The "C" locale multibyte functions: a byte is the character with the same
   code, as in ISO 8859-1, so a byte above 0x7f must not sign-extend, a
   wide character above 0xff has no multibyte form, and n bounds what is
   written, the terminator included.  Prints what fails; the exit code is
   the number of failures.  */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

static void check(int ok, const char *what)
{
  if (!ok)
    {
      printf("FAIL: %s\n", what);
      failures++;
    }
}

#define CHECK(cond) check(cond, #cond)

int main(void)
{
  static const wchar_t latin1[] = { 0xe4, 'b', 0 };
  static const wchar_t euro[] = { 'a', 0x20ac, 0 };
  wchar_t w = 0, ws[8];
  char b[8];
  int i;

  CHECK(mbtowc(&w, "\xe4", 1) == 1 && w == 0xe4);
  CHECK(mbtowc(&w, "a", 1) == 1 && w == 'a');
  CHECK(mbtowc(&w, "", 1) == 0 && w == 0);
  CHECK(mbtowc(NULL, "a", 1) == 1);
  CHECK(mbtowc(&w, "a", 0) == -1);
  CHECK(mbtowc(&w, NULL, 1) == 0);

  CHECK(mblen("\xe4xyz", 4) == 1);
  CHECK(mblen("", 1) == 0);
  CHECK(mblen("a", 0) == -1);
  CHECK(mblen(NULL, 1) == 0);

  for (i = 0; i < 8; i++)
    ws[i] = 0x55;
  CHECK(mbstowcs(ws, "\xe4\xff", 8) == 2 && ws[0] == 0xe4 && ws[1] == 0xff && ws[2] == 0);
  for (i = 0; i < 8; i++)
    ws[i] = 0x55;
  CHECK(mbstowcs(ws, "abcd", 2) == 2 && ws[1] == 'b' && ws[2] == 0x55);
  CHECK(mbstowcs(NULL, "abcd", 0) == 4);

  CHECK(wctomb(b, 0xe4) == 1 && (unsigned char) b[0] == 0xe4);
  errno = 0;
  CHECK(wctomb(b, 0x20ac) == -1 && errno == EILSEQ);
  CHECK(wctomb(NULL, 'a') == 0);

  memset(b, 0x55, sizeof(b));
  CHECK(wcstombs(b, latin1, 8) == 2 && (unsigned char) b[0] == 0xe4 && b[2] == 0);
  memset(b, 0x55, sizeof(b));
  CHECK(wcstombs(b, latin1, 1) == 1 && b[1] == 0x55);
  CHECK(wcstombs(NULL, latin1, 0) == 2);
  errno = 0;
  CHECK(wcstombs(b, euro, 8) == (size_t) -1 && errno == EILSEQ);

  return failures;
}
