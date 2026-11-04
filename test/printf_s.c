/* %s with a precision, of a string that is "0": only an integer 0 has no
   digits at precision 0, and in wide printf a char string, which may sit at
   an odd address, must not be read as wchar_t (the 68000 takes an address
   error).  Prints what fails; the exit code is the number of failures.  */

#include <stdio.h>
#include <string.h>
#include <wchar.h>

static int failures;

static void check(int ok, const char *what)
{
  if (!ok)
    {
      printf("FAIL: %s\n", what);
      failures++;
    }
}

int main(void)
{
  /* "0" and "a" at odd addresses */
  static char odd[] = "x0\0xa";
  char b[64];
  wchar_t w[64];

  snprintf(b, sizeof(b), "[%.3s][%.1s][%5.2s][%.0s]", "0", "0", "0", "0");
  check(!strcmp(b, "[0][0][    0][]"), "snprintf(\"%.3s\") of \"0\"");
  snprintf(b, sizeof(b), "[%.0d][%.0x][%.0o][%#.0o][%.0u][%.1d]", 0, 0, 0, 0, 0u, 0);
  check(!strcmp(b, "[][][][0][][0]"), "snprintf(\"%.0d\") of 0");

  swprintf(w, 64, L"[%.3s][%.3s][%.3ls]", odd + 1, odd + 4, L"0");
  check(!wcscmp(w, L"[0][a][0]"), "swprintf(\"%.3s\") of an odd char string");
  swprintf(w, 64, L"[%.0d][%#.0o]", 0, 0);
  check(!wcscmp(w, L"[][0]"), "swprintf(\"%.0d\") of 0");
  return failures;
}
