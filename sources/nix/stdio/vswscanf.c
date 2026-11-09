#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>
int vswscanf(const wchar_t *s,const wchar_t *format,va_list args)
{ FILE buffer;
  buffer._p=(unsigned char *)s;
  buffer._r=wcslen(s) * sizeof(wchar_t);
  buffer._w=0;
  buffer._flags=__SSTR|__SRD;
  buffer.tmpp=NULL;
  __STDIO_INIT_LOCK(buffer.__spinlock);
  return vfwscanf(&buffer,format,args);
}
