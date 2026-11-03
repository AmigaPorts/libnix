#include <stdlib.h>

/* Sorry, the Amiga OS locale currently doesn't support
 * character sets other than ECMA Latin 1, so this is
 * just a "C" locale function 
 */
int mbtowc(wchar_t *wc,const char *s,size_t n)
{ if(!s)
    return 0; /* no shift states */
  if(!n)
    return -1;
  if(wc)
    *wc=(unsigned char)*s;
  return *s!='\0';
}
