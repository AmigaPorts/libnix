/* strtol() and friends: the end pointer is always set, to nptr when no
   number was read; strtoll() covers the range of a long long; and
   wcstol() parses wide strings.  */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
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

/* A value no conversion stores, to catch an end pointer left unset.  */
#define UNSET ((char *)1)
#define WUNSET ((wchar_t *)1)

int main(void)
{
  static const char empty[] = "", blank[] = "   ", sign[] = "  -x";
  static const wchar_t wempty[] = L"", wnonascii[] = L"\x131";
  static const wchar_t wnum[] = L"  -17x", wtail[] = L"7\x131";
  char *end;
  wchar_t *wend;
  long long ll;
  long l;

  /* no number: the end pointer is nptr */
  end = UNSET; strtol(empty, &end, 10);
  check(end == empty, "strtol(\"\") sets end to nptr");
  end = UNSET; strtoul(empty, &end, 10);
  check(end == empty, "strtoul(\"\") sets end to nptr");
  end = UNSET; strtoll(empty, &end, 10);
  check(end == empty, "strtoll(\"\") sets end to nptr");
  end = UNSET; strtoull(empty, &end, 10);
  check(end == empty, "strtoull(\"\") sets end to nptr");
  end = UNSET; strtol(blank, &end, 10);
  check(end == blank, "strtol(\"   \") sets end to nptr");
  end = UNSET; strtoll(blank, &end, 10);
  check(end == blank, "strtoll(\"   \") sets end to nptr");
  end = UNSET; strtol(sign, &end, 10);
  check(end == sign, "strtol(\"  -x\") sets end to nptr");

  /* a number: the end pointer is just past it */
  l = strtol(" 12abc", &end, 10);
  check(l == 12 && *end == 'a', "strtol(\" 12abc\") is 12, ending at 'a'");

  /* strtoll's range is long long, not long */
  errno = 0;
  ll = strtoll("3000000000", &end, 10);
  check(ll == 3000000000LL && errno == 0, "strtoll(\"3000000000\") is 3000000000");
  errno = 0;
  ll = strtoll("-3000000000", &end, 10);
  check(ll == -3000000000LL && errno == 0, "strtoll(\"-3000000000\") is -3000000000");
  errno = 0;
  ll = strtoll("9223372036854775808", &end, 10);
  check(ll == LLONG_MAX && errno == ERANGE, "strtoll() overflow is LLONG_MAX, ERANGE");

  /* wcstol */
  wend = WUNSET;
  l = wcstol(wnum, &wend, 10);
  check(l == -17 && wend == wnum + 5, "wcstol(L\"  -17x\") is -17, ending at 'x'");
  wend = WUNSET; wcstol(wempty, &wend, 10);
  check(wend == wempty, "wcstol(L\"\") sets end to nptr");
  wend = WUNSET;
  l = wcstol(wnonascii, &wend, 10);
  check(l == 0 && wend == wnonascii, "wcstol() reads no digit from U+0131");
  wend = WUNSET;
  l = wcstol(wtail, &wend, 10);
  check(l == 7 && wend == wtail + 1, "wcstol(L\"7\\x131\") is 7, ending at U+0131");

  printf("%d failures\n", failures);
  return failures;
}
