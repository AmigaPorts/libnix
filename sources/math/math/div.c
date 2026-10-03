#include <stdlib.h>

asm(".global _ldiv; _ldiv = _div");
div_t div(int numer, int denom) {
	div_t r;
	r.quot = numer / denom;
	r.rem = numer % denom;
	return r;
}
