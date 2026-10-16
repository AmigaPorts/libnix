#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "stdio.h"

/* Format into a buffer, then write() it; sockets may take it in pieces. */
int vdprintf(int fd, const char *format, va_list args) {
	char small[256];
	char *buf = small;
	va_list copy;
	int len;
	size_t done = 0;

	va_copy(copy, args);
	len = vsnprintf(small, sizeof(small), format, copy);
	va_end(copy);
	if (len < 0)
		return len;
	if ((size_t)len >= sizeof(small)) {
		buf = malloc((size_t)len + 1);
		if (!buf) {
			errno = ENOMEM;
			return -1;
		}
		vsnprintf(buf, (size_t)len + 1, format, args);
	}
	while (done < (size_t)len) {
		ssize_t n = write(fd, buf + done, len - done);
		if (n <= 0) {
			len = -1;
			break;
		}
		done += n;
	}
	if (buf != small)
		free(buf);
	return len;
}
