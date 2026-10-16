#include "stdio.h"

/* off_t is long here, so this is ftell. */
off_t ftello(FILE *stream) {
	return ftell(stream);
}
