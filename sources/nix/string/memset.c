#include <string.h>

#define NNN 18

/* The stores are asm statements, so that gcc can't turn the loops back
   into a call to memset(). */
void *memset(void *s, int _c, size_t n) {
	unsigned long c = (unsigned char) _c;
	char *p = n + (char *) s;

	if (n > NNN) {
		c |= c << 8;
		c |= c << 16;

		if ((long) p & 1) {
			asm volatile("move.b %1,-(%0)" : "+a" (p) : "d" (c) : "memory");
			--n;
		}
		if ((long) p & 2) {
			asm volatile("move.w %1,-(%0)" : "+a" (p) : "d" (c) : "memory");
			n -= 2;
		}
		size_t m = n / (8 * sizeof(long));
		/* One statement, so that nothing can reuse the registers between
		   loading them and storing them. */
		if (m) {
			asm volatile(
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
				: "+a" (p), "+&d" (m)
				: "d" (c)
				: "d5", "d6", "d7", "a1", "a2", "a3", "a6", "memory");
			n &= (8 * sizeof(long) - 1);
		}
		{
			size_t k = n >> 2;
			for (; k; --k) {
				asm volatile("move.l %1,-(%0)" : "+a" (p) : "d" (c) : "memory");
			}
			n &= sizeof(long) - 1;
		}
	}

	while(n--) {
		asm volatile("move.b %1,-(%0)" : "+a" (p) : "d" (c) : "memory");
	}

	return p;
}
