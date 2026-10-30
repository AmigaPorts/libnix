#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
int fwscanf(FILE *stream,const wchar_t *format,...)
{ int retval;
  va_list args;
  va_start(args,format);
  retval=vfwscanf(stream,format,args);
  va_end(args);
  return retval;
}
