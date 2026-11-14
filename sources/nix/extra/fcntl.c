#include <fcntl.h>
#include <stdarg.h>
#include "stdio.h"

/* The socket library has its own, which comes first: see __fcntl().  */
int __stdargs fcntl(int fd, int cmd, ... /* arg */ )
{
	va_list va;
	long arg;

	va_start(va, cmd);
	arg = va_arg(va, long);
	va_end(va);
	return __fcntl(fd, cmd, arg);
}
