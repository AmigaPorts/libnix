#define __NO_INLINE__
#include <string.h>
__stdargs size_t strnlen(const char *string, size_t maxlen) {
	const char *s = string;

	while (maxlen-- && *s) {
		s++;
	}
	return s - string;
}
