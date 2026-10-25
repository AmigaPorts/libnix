/* fmaf: x * y + z, rounded once (in round-to-nearest).
 *
 * From musl (src/math/fmaf.c). The product of two floats is exact in
 * double, and the sum is fixed up when its rounding to double could make
 * the rounding to float wrong. The double and float results are volatile:
 * libm is built with -ffast-math, and on a 68881 they would otherwise stay
 * in extended precision.
 *
 * Copyright (c) 2005-2020 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <math.h>
#include <stdint.h>

float fmaf(float x, float y, float z)
{
	double xy = (double)x * y;
	volatile union {double r; uint64_t i;} u = {xy + z};
	volatile float f;
	int e = u.i>>52 & 0x7ff;
	/* covers |r| > 0x1p-126 halfway cases (may round incorrectly) */
	int halfway = (u.i & 0x1fffffff) == 0x10000000;
	/* covers tiny inexact (may miss uflow) and tiny halfway cases */
	int tiny = e <= 0x3ff-126 && e >= 0x3ff-149;
	if (!halfway && !tiny) {
		/* common case, optimization only */
		f = u.r;
		return f;
	}
	if (e != 0x7ff) {
		/* r+t == x*y+z exactly in nearest rounding, otherwise
		 * rounding does not affect t!=0, t<0 and inexact flag. */
		int s = u.i >> 63;
		double t = s == (xy < z) ? xy - u.r + z : z - u.r + xy;
		if (t) {
			/* adjust r toward r+t if r%2==0 (round to odd).
			 * may be needed if r is halfway or exact float,
			 * wrong if r becomes halfway or exact float. */
			u.i -= s ^ (t<0);
			u.i |= 1;
		}
	}
	f = u.r;
	return f;
}
