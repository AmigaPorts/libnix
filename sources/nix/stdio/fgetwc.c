#include <errno.h>
#include <stdlib.h>
#include <wchar.h>
#include "stdio.h"

wint_t fgetwc(FILE *fp) {
	wint_t b;
	if (fread((char *) &b, sizeof(wint_t), 1, fp) > 0)
		return b;
	return WEOF;
}
