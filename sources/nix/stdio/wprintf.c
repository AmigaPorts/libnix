#include <stdarg.h>
#include "stdio.h"
#include "wchar.h"

int wprintf(const wchar_t *format,...)
{ int retval;
  va_list args;
  va_start(args,format);
  retval=vfwprintf(stdout, format, args);
  va_end(args);
  return retval;
}
