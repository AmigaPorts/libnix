#include <stdarg.h>
#include "stdio.h"
#include <errno.h>

extern int __vfprintf_total_size(FILE *stream, const char *fmt, va_list args);

int vsnprintf(char *s,size_t size,const char *format,va_list args) {
	size_t retval;
	FILE buffer;

	int saved_errno = errno;

	if (!s && size)
		return EOF;

	/* size 0, with or without a buffer (the C99 sizing idiom), means no room
	 * at all: putc goes straight to __swbuf, which never touches _p. */
	buffer._p=(unsigned char *)s;
	buffer._r=0;
	buffer._w=size ? (int)(size-1) : -1;
	buffer._flags=__SSTR|__SWR;
	buffer.linebufsize=0;
#ifdef __posix_threads__
	buffer.__spinlock[0] = 0;
#endif
	retval=__vfprintf_total_size(&buffer,format,args);
	if (retval < size)
		s[retval] = 0;
	else if (size > 0)
		s[size - 1] = 0;
	/* __swbuf sets errno = EPERM when a string stream fills; that is not an
	 * error the caller made, so it must not leak out of a truncating call. */
	errno = saved_errno;
	return retval;
}
