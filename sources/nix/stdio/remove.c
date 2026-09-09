#include <errno.h>
#include <proto/dos.h>
#include "stdio.h"
#include "amigapath.h"

extern void __seterrno(void);

asm("_rmdir: .global _rmdir");
asm("_unlink: .global _unlink");
int remove(const char *filename)
{ 
  if((filename=__amigapath(filename))==NULL)
    return -1;
  if(DeleteFile((CONST_STRPTR)filename))
    return 0;
  else
  { __seterrno(); return -1; }
}
