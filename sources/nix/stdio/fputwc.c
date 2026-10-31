#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <wchar.h>
#include "stdio.h"

wint_t fputwc(wchar_t wc, FILE *fp) {
	if (1 == fwrite(&wc, sizeof(wchar_t), 1, fp))
		return wc;
	return WEOF;
}
