#include <stdarg.h>
#include "stdio.h"

int dprintf(int fd, const char *format, ...) {
	va_list args;
	int r;

	va_start(args, format);
	r = vdprintf(fd, format, args);
	va_end(args);
	return r;
}
