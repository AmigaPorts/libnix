#include <string.h>

asm(".global _rindex; _rindex = _strrchr");
char *strrchr(const char *s,int c)
{ char *c1=(char *)0;
  do
    if (*s==(char)c)
      c1=(char *)s;
  while(*s++);
  return c1;
}
