#undef __NO_INLINE__
#define __NO_INLINE__
#include <string.h>
#include <wchar.h>

wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n) {
	wchar_t *v = s + n;
	size_t m = n / 8;

	/* One statement, so that nothing can reuse the registers between
	   loading them and storing them. */
	if (m) {
		__asm volatile(
			"move.l %2,d5\n\t"
			"move.l %2,d6\n\t"
			"move.l %2,d7\n\t"
			"move.l %2,a1\n\t"
			"move.l %2,a2\n\t"
			"move.l %2,a3\n\t"
			"move.l %2,a6\n"
			"1:\tmovem.l %2/d5/d6/d7/a1/a2/a3/a6,-(%0)\n\t"
			"subq.l #1,%1\n\t"
			"jne 1b"
			: "+a" (v), "+&d" (m)
			: "d" (c)
			: "d5", "d6", "d7", "a1", "a2", "a3", "a6", "memory");
		n &= 7;
	}
	while (n--)
		*--v = c;
	return s;
}
