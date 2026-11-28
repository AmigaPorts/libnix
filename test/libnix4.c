/* A program on libnix4.library, -mcrt=library: the C library is the shared
   library's, which the startup code opens and links the program's calls to.
   Calls into it, the library calling back into the program (qsort()'s
   comparison), and the per-opener data the library keeps for this program
   (errno, the heap, stdio's buffers).  Prints what fails; the exit code is
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

static int compare(const void *a, const void *b)
{
  return *(const int *) a - *(const int *) b;
}

int main(void)
{
  char buf[64];

  printf("Hello, world!\n");

  /* formatting, integer and floating point */
  snprintf(buf, sizeof(buf), "%d %s %lx %.3f", -42, "libnix", 0xdeadbeefUL, 3.14159);
  check(strcmp(buf, "-42 libnix deadbeef 3.142") == 0, "snprintf");

  /* the heap and the string functions */
  char *s = malloc(100);
  check(s != NULL, "malloc");
  if (s)
    {
      strcpy(s, "shared ");
      strcat(s, "library");
      check(strlen(s) == 14 && strcmp(s, "shared library") == 0, "strcpy, strcat, strlen");
      s = realloc(s, 1000);
      check(s != NULL && strcmp(s, "shared library") == 0, "realloc");
      free(s);
    }

  /* a callback from the library into the program */
  int v[] = { 5, 3, 9, 1, 7, 2, 8 };
  qsort(v, sizeof(v) / sizeof(v[0]), sizeof(v[0]), compare);
  check(v[0] == 1 && v[3] == 5 && v[6] == 9, "qsort");

  /* errno, which the library keeps for this program */
  check(strtol("12345", NULL, 10) == 12345, "strtol");
  errno = 0;
  strtol("99999999999999999999", NULL, 10);
  check(errno == ERANGE, "errno from strtol");

  if (failures == 0)
    puts("PASS");
  return failures;
}
