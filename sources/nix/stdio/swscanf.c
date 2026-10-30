#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
int swscanf(const wchar_t *s,const wchar_t *format,...)
{ int retval;
  va_list args;
  va_start(args,format);
  retval=vswscanf(s,format,args);
  va_end(args);
  return retval;
}
