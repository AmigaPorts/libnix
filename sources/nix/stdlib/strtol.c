#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include "stdio.h"
#include "stabs.h"

signed long strtol(const char *nptr,char **endptr,int base)
{ const char *p=nptr;
  char *q;
  unsigned long r;
  int olderrno=errno,overflow;
  if(!(nptr && *nptr))
  { if(endptr!=NULL)
      *endptr=(char *)nptr;
    errno=EINVAL;
    return 0;
  }
  while(isspace((int)*p))
    p++;
  /* strtoul's overflow is ULONG_MAX, unnegated: the same value as "-1" */
  errno=0;
  r=strtoul(p,&q,base);
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
  { if(overflow || (signed long)r>0)
    { errno=ERANGE;
      return LONG_MIN; }
    else
      return r;
  }else
  { if((signed long)r<0)
    { errno=ERANGE;
      return LONG_MAX; }
    else
      return r;
  }
}
