#define __NO_INLINE__
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
__stdargs int vwscanf(const wchar_t *format, va_list args) {
	return vfwscanf(stdin, format, args);
}
