#include <stdlib.h>
#include <errno.h>

/* Sorry, the Amiga OS locale currently doesn't support
 * character sets other than ECMA Latin 1, so this is
 * just a "C" locale function
 */
int wctomb(char *s,wchar_t wc)
{ if(!s)
    return 0; /* no shift states */
  if((unsigned long)wc>0xff)
  { errno=EILSEQ;
    return -1; }
  *s=wc;
  return 1;
}
