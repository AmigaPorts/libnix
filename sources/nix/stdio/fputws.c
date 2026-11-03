#include <wchar.h>
#include "stdio.h"

int fputws(const wchar_t *ws, FILE *fp)
{
    size_t len = wcslen(ws);

    if (fwrite(ws, sizeof(wchar_t), len, fp) != len)
        return -1;

    return 0;
}
