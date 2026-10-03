#include <string.h>
asm(".global _strupr; _strupr = _strupper");
char *strupper(char *s)
{ unsigned char *s1=(unsigned char *)s;

  while(*s1) {
    if ((*s1>('a'-1)) && (*s1<('z'+1)))
      *s1-='a'-'A';
    s1++;
  }
  return s;
}
