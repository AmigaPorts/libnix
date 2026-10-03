#include <string.h>
#include <proto/utility.h>

asm(".global _strcasecmp; _strcasecmp = _stricmp");
int stricmp(const char *s1,const char *s2)
{
  return Stricmp((STRPTR)s1,(STRPTR)s2);
}
