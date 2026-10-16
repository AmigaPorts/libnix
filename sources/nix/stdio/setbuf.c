#define __NO_INLINE__
#include "stdio.h"
__stdargs void setbuf(FILE *stream, char *buf) {
	setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}
