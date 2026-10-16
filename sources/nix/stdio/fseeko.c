#include "stdio.h"

/* off_t is long here, so this is fseek. */
int fseeko(FILE *stream, off_t offset, int whence) {
	return fseek(stream, offset, whence);
}
