#define __NO_INLINE__
#include <ctype.h>
#include <string.h>

size_t strlcpy(char *dst, const char *src, size_t dsize) {
	const char *p = src;

	while (*p) {
		++p;
	}
	size_t srclen = p - src;

	if (dsize != 0) {
		while (--dsize != 0) {
			if ((*dst++ = *src++) == '\0') {
				break;
			}
		}
		if (dsize == 0) {
			*dst = '\0';
		}
	}

	return srclen;
}
