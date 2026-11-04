#include <stdlib.h>
#include <errno.h>

/* Sorry, the Amiga OS locale currently doesn't support
 * character sets other than ECMA Latin 1, so this is
 * just a "C" locale function
 */
size_t wcstombs(char *s,const wchar_t *wc,size_t n)
{ size_t l=0;
  if(!s) /* only count */
    n=(size_t)-1;
  for(;l<n;l++)
  { if((unsigned long)wc[l]>0xff)
    { errno=EILSEQ;
      return (size_t)-1; }
    if(s)
      s[l]=wc[l];
    if(!wc[l])
      break; }
  return l;
}
