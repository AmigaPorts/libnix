#include <stdlib.h>

/* Sorry, the Amiga OS locale currently doesn't support
 * character sets other than ECMA Latin 1, so this is
 * just a "C" locale function 
 */
size_t mbstowcs(wchar_t *wc,const char *s,size_t n)
{ size_t l=0;
  if(!wc) /* only count */
    n=(size_t)-1;
  for(;l<n;l++)
  { unsigned char c=s[l];
    if(wc)
      wc[l]=c;
    if(!c)
      break; }
  return l;
}
