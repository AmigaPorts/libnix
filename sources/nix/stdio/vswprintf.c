#include <stdarg.h>
#include "stdio.h"
#include "wchar.h"

extern int __vfwprintf_total_size(FILE *stream, const wchar_t *fmt, va_list args);

int vswprintf(wchar_t *s, size_t size, const wchar_t *format, va_list args) {
	int retval;
	FILE buffer;

	if (!s && size)
		return EOF;

	buffer._p = (unsigned char *)s;
	buffer._r = 0;
	/* _w is the number of writable BYTES.  Reserve room for the
	 * wchar_t terminator: (size - 1) wide chars, each sizeof(wchar_t)
	 * bytes, when size > 0. */
	buffer._w = (size > 0) ? (int)((size - 1) * sizeof(wchar_t)) : 0;
	buffer._flags = __SSTR | __SWR;
	buffer.linebufsize = 0;
#ifdef __posix_threads__
	buffer.__spinlock[0] = 0;
#endif

	/* __vfwprintf_total_size returns the number of WIDE CHARACTERS that
	 * would have been written (C99), not bytes.  Do not divide by
	 * sizeof(wchar_t) here. */
	retval = __vfwprintf_total_size(&buffer, format, args);

	if (retval < 0) {
		/* A real I/O or encoding error.  Still NUL-terminate if we can. */
		if (size > 0)
			s[size - 1] = 0;
		return -1;
	}

	if ((size_t)retval >= size) {
	    if (size > 0)
	        s[size - 1] = 0;
	    return -1;
	}
	s[retval] = 0;
	return retval;
}
