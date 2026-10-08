#define __NO_INLINE__
#include "stdio.h"
__stdargs int fsetpos(FILE *stream, const fpos_t *pos) {
	return fseek(stream, *pos, SEEK_SET);
}
