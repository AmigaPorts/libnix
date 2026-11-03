#include <errno.h>
#include <stdlib.h>
#include <wchar.h>
#include "stdio.h"

wint_t fgetwc(FILE *fp)
{
    wchar_t wc;

    if (fread(&wc, sizeof(wchar_t), 1, fp) != 1)
        return WEOF;

    return wc;
}
