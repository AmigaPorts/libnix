#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
int wscanf(const wchar_t *format,...)
{ int retval;
  va_list args;
  va_start(args,format);
  retval=vwscanf(format,args);
  va_end(args);
  return retval;
}
