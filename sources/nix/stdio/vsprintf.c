#include <stdarg.h>
#include <limits.h>
#include "stdio.h"

int vsprintf(char *s, const char *format, va_list args) {
	int retval;
	FILE buffer;
	buffer._p = (unsigned char *)s;
	buffer._r = 0;
	buffer._w = INT_MAX;
	buffer._flags = __SSTR | __SWR;
	buffer.linebufsize = 0;
#ifdef __posix_threads__
	buffer.__spinlock[0] = 0;
#endif
	retval = vfprintf(&buffer, format, args);
	/* directly, as vsnprintf does: fputc with _w = retval wrote nothing
	   after an empty output */
	if (retval >= 0)
		s[retval] = '\0';
	return retval;
}
