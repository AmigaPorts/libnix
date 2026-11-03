#undef __NO_INLINE__
#define __NO_INLINE__
#include <string.h>
#include <wchar.h>

wchar_t *wmemset(wchar_t *s, wchar_t _c, size_t _n) {
	if (_n) {
		register unsigned c __asm("d0") = _c;
		size_t n = _n;
		register wchar_t * v __asm("a0") = s + n;

		size_t m = n / 8;
		if (m) {
			__asm volatile("move.l %0,a6" :: "r" (c) : "a6");
			__asm volatile("move.l %0,d5" :: "r" (c) : "d5");
			__asm volatile("move.l %0,d6" :: "r" (c) : "d6");
			__asm volatile("move.l %0,d7" :: "r" (c) : "d7");
			__asm volatile("move.l %0,a1" :: "r" (c) : "a1");
			__asm volatile("move.l %0,a2" :: "r" (c) : "a2");
			__asm volatile("move.l %0,a3" :: "r" (c) : "a3");
			while (m--) {
				__asm volatile("movem.l %1/d5/d6/d7/a1/a2/a3/a6,-(%0)" : "+r" (v) : "r" (c) : "memory");
			}
			n &= 7;
		}
		for (; n; --n)
			*--v = c;
	}
	return s;
}
