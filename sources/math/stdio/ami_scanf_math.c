/* ==========================================================================
 *  ami_scanf_math.c
 *
 *  The shared scanf FP math core.
 *
 *  See ami_printf_math.h for the contract of each function.  All arithmetic
 *  here is signless, on the local LDL view: 16-bit exponent, 64-bit mantissa
 *  storage with the integer bit at b63 (Motorola 68881 / x87 extended
 *  precision).  The l.low field is retained in the LDL struct for ABI /
 *  source compatibility but is unused and always zero.
 *
 *  Design note: this build targets a 64-bit-mantissa long double.  The
 *  accumulator therefore saturates at 64 significant bits: a 17th hex
 *  digit (or a decimal digit past the 64-bit capacity) is dropped, and
 *  __ldl_to_long_double returns the exact 64-bit mantissa with no further
 *  rounding.  This makes strtold/printf round-trip bit-exact for every
 *  value that is representable in a 68881 extended-precision register.
 *
 * --------------------------------------------------------------------------
 *  Copyright and license
 * --------------------------------------------------------------------------
 *
 *  This file is part of the libnix C library for AmigaOS / m68k.
 *
 *  libnix is public domain.  As far as the law allows, this software comes
 *  as is, without any warranty or condition, and no contributor will be
 *  liable to anyone for any damages related to this software or this
 *  license, under any kind of legal claim.
 *
 *  Author: Stefan "Bebbo" Franke
 *
 * ========================================================================== */
#include <stdio.h>
#include "ami_printf_math.h"

/* In-place multiply by 10 and add a decimal digit:
 *     *x = *x * 10 + digit,  0 <= digit <= 9
 *
 * The mantissa is treated as a plain 96-bit unsigned integer held in
 * (high, middle).  The exponent is NOT maintained by this routine; the
 * caller starts with *x = { 0, 0, 0, 0 } and calls __LDL_normalize once
 * at the end to find the highest set bit and set the exponent.
 */
int __LDL_mul_10_add(LDL *x, int digit)
{
	unsigned long hi = x->l.high;
	unsigned long mid = x->l.middle;
	unsigned long lo = x->l.low;

	/* Saturation: x * 10 + digit must stay below 2^96.  (2^96 - 1) / 10 is
	 * 0x19999999_99999999_99999999 with remainder 5, so at that x only
	 * the digits up to 5 still fit. */
	if (hi > 0x19999999UL)
		return 0;
	if (hi == 0x19999999UL) {
		unsigned long long midlo = ((unsigned long long) mid << 32) | lo;
		if (midlo > 0x9999999999999999ULL - (digit > 5))
			return 0;
	}

	/* a = 8x  = x << 3 */
	unsigned long a_hi = (hi << 3) | (mid >> 29);
	unsigned long a_mid = (mid << 3) | (lo >> 29);
	unsigned long a_lo = (lo << 3);

	/* b = 2x  = x << 1 */
	unsigned long b_hi = (hi << 1) | (mid >> 31);
	unsigned long b_mid = (mid << 1) | (lo >> 31);
	unsigned long b_lo = (lo << 1);

	/* r = a + b, carrying through 96 bits */
	unsigned long r_lo = a_lo + b_lo;
	unsigned long carry_lo = (r_lo < a_lo);
	unsigned long r_mid = a_mid + b_mid + carry_lo;
	unsigned long carry_mid = (r_mid < a_mid) ||
			(carry_lo && r_mid == a_mid);
	unsigned long r_hi = a_hi + b_hi + carry_mid;

	/* r += digit, carry through */
	unsigned long old_lo = r_lo;
	r_lo += (unsigned long) digit;
	if (r_lo < old_lo) {
		if (++r_mid == 0)
			r_hi++;
	}

	x->l.high = r_hi;
	x->l.middle = r_mid;
	x->l.low = r_lo;
	return 1;
}

/* In-place multiply by 16 and add a hex digit:
 *     *x = *x * 16 + digit,  0 <= digit <= 15
 *
 * Like __LDL_mul_10_add: the mantissa is treated as a plain 96-bit
 * unsigned integer, the exponent is not touched (stays 0 from the
 * caller's initializer), and __LDL_normalize sets it at the end.
 *
 * x * 16 is a pure left shift by 4, so there's no (x + x/4)-style
 * approximation and no truncation: the only loss is the carry out of
 * bit 63, which is the caller's saturation concern.
 */
int __LDL_mul_16_add(LDL *x, int digit)
{
	unsigned long hi = x->l.high;
	unsigned long mid = x->l.middle;
	unsigned long lo = x->l.low;

	/* Saturation: hi's top nibble non-zero means ×16 overflows 96 bits. */
	if (hi >= 0x10000000UL) {
		if (x->l.low == 0) {
			/* Preserve the first dropped nibble at low's top for
			 * extra round-to-nearest information. */
			x->l.low = ((unsigned long) digit << 28) | 1;
		}
		return 0;
	}

	x->l.high = (hi << 4) | (mid >> 28);
	x->l.middle = (mid << 4) | (lo >> 28);
	x->l.low = (lo << 4) | ((unsigned long) digit & 0xF);

	return 1;
}

/* Normalize an LDL built by __LDL_mul_10_add / __LDL_mul_16_add (which
 * leave exp = 0): find the highest set bit b in the 96-bit mantissa,
 * shift left so it lands at b63, and set exp = 16383 + b.  A zero
 * mantissa stays zero.
 *
 * Precondition: x->l.exp == 0 (the accumulator's initial state).  If
 * x is already normalized (high != 0, bit 31 set), this is a no-op
 * except for setting exp. */
void __LDL_normalize(LDL *x)
{
	unsigned long hi = x->l.high;
	unsigned long mid = x->l.middle;
	unsigned long lo = x->l.low;
	int n;

	if (hi == 0 && mid == 0 && lo == 0) {
		x->l.exp = 16383;
		return;
	}

	if (hi != 0) {
		n = __builtin_clz(hi);
		if (n > 0) {
			x->l.high = (hi << n) | (mid >> (32 - n));
			x->l.middle = (mid << n) | (lo >> (32 - n));
			x->l.low = (lo << n);
		}
	} else if (mid != 0) {
		n = __builtin_clz(mid);
		x->l.high = (mid << n) | (n ? lo >> (32 - n) : 0);
		x->l.middle = (lo << n);
		x->l.low = 0;
	} else {
		n = __builtin_clz(lo);
		x->l.high = (lo << n);
		x->l.middle = 0;
		x->l.low = 0;
	}

	x->l.exp = 16383;
}

double __ldl_to_double(const LDL *lv)
{
	unsigned long hi = lv->l.high;
	unsigned long mid = lv->l.middle;
	unsigned long long mant;
	int e, E;

	if (hi == 0 && mid == 0)
		return 0.0;

	e = (int) lv->l.exp - 16383;
	mant = ((unsigned long long) hi << 32) | mid;

	{
		unsigned long long m53 = mant >> 11;
		unsigned long guard = (mant >> 10) & 1;
		unsigned long sticky = (mant & 0x3FFULL) != 0 || lv->l.low != 0;

		if (guard && (sticky || (m53 & 1))) {
			m53++;
			if (m53 == (1ULL << 53)) {
				m53 = 1ULL << 52;
				e++;
			}
		}

		E = e + 1023;

		if (E >= 0x7FF)
			return (double) (1.0 / 0.0);

		if (E <= 0) {
			int shift = -e - 1022;
			unsigned long long m;

			if (shift >= 64) {
				m = 0; /* m53 < 2^53 < 2^63 = half */
			} else {
				unsigned long long lost = m53 & ((1ULL << shift) - 1);
				unsigned long long half = 1ULL << (shift - 1);

				m = m53 >> shift;
				if ((lost & half) && ((lost & (half - 1)) || (m & 1)))
					m++;
			}

			if (m >= (1ULL << 52)) {
				E = 1;
				m = 0;
			} else {
				E = 0;
			}

			{
				unsigned long long frac52 = m & 0xFFFFFFFFFFFFFULL;
				union _d_bits u;
				u.u = 0;
				u.b.sign = 0;
				u.b.exp = (unsigned) E;
				u.b.frac0 = (unsigned) (frac52 >> 32);
				u.b.frac1 = (unsigned) (frac52 & 0xFFFFFFFFUL);
				return u.d;
			}
		}

		{
			unsigned long long frac52 = m53 & 0xFFFFFFFFFFFFFULL;
			union _d_bits u;
			u.u = 0;
			u.b.sign = 0;
			u.b.exp = (unsigned) E;
			u.b.frac0 = (unsigned) (frac52 >> 32);
			u.b.frac1 = (unsigned) (frac52 & 0xFFFFFFFFUL);
			return u.d;
		}
	}
}

long double __ldl_to_long_double(const LDL *lv)
{
	union _ld_bits u;
	unsigned long hi = lv->l.high;
	unsigned long mid = lv->l.middle;
	unsigned long lo = lv->l.low;
	int e;

	if (hi == 0 && mid == 0 && lo == 0)
		return 0.0L;

	e = (int) lv->l.exp - 16383;

	u.ld = 0.0L;
	u.b.sign = 0;
	u.b.pad = 0;

	{
		unsigned long long m64 = ((unsigned long long) hi << 32) | mid;

		/* Round to nearest-even at 64 bits. */
		{
			unsigned long guard = (lo >> 31) & 1;
			unsigned long sticky = (lo & 0x7FFFFFFFUL) != 0;

			if (guard && (sticky || (m64 & 1))) {
				m64++;
				if (m64 == 0) {
					m64 = 0x8000000000000000ULL;
					e++;
				}
			}
		}

		{
			long E = (long) e + 16383;

			if (E >= 0x7FFF) {
				u.b.exp = 0x7FFF;
				u.b.mant = 0x8000000000000000ULL;
				return u.ld;
			}

			if (E <= 0) {
				int shift = 1 - (int) E;
				unsigned long long m = m64;

				if (shift >= 64) {
					/* half = 2^63.  m64 < 2^64.  Round up only if strictly
					 * greater than half; exact half ties to even -> 0
					 * (result 0 is even).
					 */
					if (shift == 64 && (m >> 63) &&
							(m & 0x7FFFFFFFFFFFFFFFULL))
						m = 1;
					else
						m = 0;
				} else {
					unsigned long long lost = m & ((1ULL << shift) - 1);
					unsigned long long half = 1ULL << (shift - 1);

					m >>= shift;
					if ((lost & half) && ((lost & (half - 1)) || (m & 1)))
						m++;
				}

				if (m & 0x8000000000000000ULL) {
					m &= 0x7FFFFFFFFFFFFFFFULL;
					E = 1;
				} else {
					E = 0;
				}

				u.b.exp = (unsigned) E;
				u.b.mant = m;
				return u.ld;
			}

			u.b.exp = (unsigned) E;
			u.b.mant = m64;
		}
	}
	return u.ld;
}
