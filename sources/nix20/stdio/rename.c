#include <string.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include "stdio.h"
#include "amigapath.h"

extern void __seterrno(void);

int rename(const char *old,const char *new)
{
  int ret=-1;
  BPTR lnew;

  if((old=__amigapath(old))==NULL || (new=__amigapath(new))==NULL)
    return -1;

  lnew=Lock((CONST_STRPTR)new,SHARED_LOCK);
  if(lnew) {
    BPTR lold=Lock((CONST_STRPTR)old,SHARED_LOCK);
    if(lold)
      ret=SameLock(lold,lnew),UnLock(lold);
    UnLock(lnew);
  }

  if(ret) {
    if(ret==1)
      DeleteFile((CONST_STRPTR)new);
    if(ret=0,!Rename((CONST_STRPTR)old,(CONST_STRPTR)new))
      __seterrno(),ret=-1;
  }

  return ret;
}
