#include <stddef.h>

/*
 * Appends src to string dst of size dsize (unlike strncat, dsize is the
 * full size of dst, not space left). At most dsize-1 characters
 * will be copied. Always NULL-terminates (unless dsize == 0).
 * Returns strlen(src) + MIN(dsize, strlen(initial dst)).
 */
size_t strlcat(char *dst, const char *src, size_t dsize) {
    const char *odest = dst;
    const char *osrc = src;
    size_t n = dsize;
    size_t dlen;

    /* Find the end of dst and adjust bytes left */
    while (n-- != 0 && *dst != '\0')
        dst++;
    dlen = dst - odest;
    n = dsize - dlen;

    if (n == 0)
        return (dlen + strlen(osrc));
    while (*src != '\0') {
        if (n != 1) {
            *dst++ = *src;
            n--;
        }
        src++;
    }
    *dst = '\0';

    return (dlen + (src - osrc));
}
