#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include "stdio.h"

#ifndef LLONG_MIN
#define LLONG_MIN 0x8000000000000000L
#endif
#ifndef LLONG_MAX
#define LLONG_MAX 0x7fffffffffffffffL
#endif

asm("_strtoimax: .global _strtoimax");
signed long long strtoll(const char *nptr,char **endptr,int base)
{ const char *p=nptr;
  char *q;
  unsigned long long r;
  int olderrno=errno,overflow;
  if(!(nptr && *nptr))
  { if(endptr!=NULL)
      *endptr=(char *)nptr;
    errno=EINVAL;
    return 0;
  }
  while(isspace((int)*p))
    p++;
  /* strtoull's overflow is ULLONG_MAX, unnegated: the same value as "-1" */
  errno=0;
  r=strtoull(p,&q,base);
  overflow=errno==ERANGE;
  if(!errno)
    errno=olderrno;
  if(endptr!=NULL)
  { if(q==p)
      *endptr=(char *)nptr;
    else
      *endptr=q;
  }
  if(*p=='-')
  { if(overflow || (signed long long)r>0)
    { errno=ERANGE;
      return LLONG_MIN; }
    else
      return r;
  }else
  { if((signed long long)r<0)
    { errno=ERANGE;
      return LLONG_MAX; }
    else
      return r;
  }
}
