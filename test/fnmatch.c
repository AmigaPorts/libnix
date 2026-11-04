/* fnmatch() bracket expressions, with the results glibc gives: a backslash
   escapes inside the brackets too, so an escaped ']' doesn't close them,
   and the pattern is never read past its end, whatever follows it.  Prints
   what fails; the exit code is the number of failures.  */

#include <fnmatch.h>
#include <stdio.h>
#include <string.h>

static const struct {
  const char *pattern, *string;
  int flags, nomatch;
} cases[] = {
  { "[a-z]", "m", 0, 0 },
  { "[a-z]", "M", 0, 1 },
  { "[a-z]", "M", FNM_CASEFOLD, 0 },
  { "[!a]", "b", 0, 0 },
  { "[^a]", "a", 0, 1 },
  { "[]]", "]", 0, 0 },
  { "[!]]", "]", 0, 1 },
  { "[\\]]", "]", 0, 0 },
  { "[\\]]", "\\]", 0, 1 },
  { "[a\\-z]", "-", 0, 0 },
  { "[a\\-z]", "m", 0, 1 },
  { "[\\a-c]", "b", 0, 0 },
  { "[a-\\c]", "b", 0, 0 },
  { "[a-\\]", "a", 0, 1 },
  { "[a-\\]", "[a-]", 0, 0 },
  { "[a-]", "-", 0, 0 },
  { "[\\\\]", "\\", 0, 0 },
  { "[\\]", "\\", 0, 1 },
  { "[\\]", "[\\]", 0, 1 },
  { "[a", "[a", 0, 0 },
  { "[\\]]", "]", FNM_NOESCAPE, 1 },
  { "[\\]]", "\\]", FNM_NOESCAPE, 0 },
  { "[a-\\]", "\\", FNM_NOESCAPE, 1 },
  { "[[:digit:]]", "5", 0, 0 },
  { "[[:alpha:]0]", "0", 0, 0 },
  { "[[:digit:]", "[[:digit:]", 0, 1 },
  { "x[[:digit:]\\]]y", "x]y", 0, 0 },
};

int main(void)
{
  static char buf[16];
  int failures = 0;
  unsigned i;

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    if ((fnmatch(cases[i].pattern, cases[i].string, cases[i].flags) != 0) != cases[i].nomatch)
      {
        printf("FAIL: fnmatch(\"%s\", \"%s\", %d) should be %s\n", cases[i].pattern, cases[i].string,
               cases[i].flags, cases[i].nomatch ? "FNM_NOMATCH" : "0");
        failures++;
      }

  /* whatever follows the pattern's NUL can't make it match */
  strcpy(buf, "[a-\\]");
  memcpy(buf + 6, "a]", 2);
  if (fnmatch(buf, "a", 0) == 0)
    {
      printf("FAIL: fnmatch(\"[a-\\\\]\", \"a\", 0) read past the pattern\n");
      failures++;
    }
  return failures;
}
