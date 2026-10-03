#include <string.h>

asm(".global _index; _index = _strchr");
char *strchr(const char *s,int c)
{
  while (*s!=(char)c)
    if (!*s++)
      { s = (char *)0; break; }
  return (char *)s;
}
