/*
 * ==========================================================================
 *  ami_scanf_math.c
 *
 *  Shared scanf floating-point math core.
 *
 *  See ami_printf_math.h for the common LDL representation and function
 *  contracts. All arithmetic operates on signless values using a 96-bit
 *  mantissa and a signed 32-bit exponent.
 *
 *  The accumulator uses all three mantissa words:
 *    high   : bits 95..64
 *    middle : bits 63..32
 *    low    : bits 31..0
 *
 *  Decimal and hexadecimal input are accumulated as integers. The
 *  accumulator is normalized after digit collection, and the exponent
 *  is adjusted separately by the parsing code.
 *
 *  Conversion to the native Motorola extended format retains 64
 *  significant bits and rounds using the remaining 32 bits and the
 *  additional sticky information supplied by the parser.
 *
 *  The implementation is software-only and does not require FPU
 *  arithmetic.
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
#include "ami_printf_math.h"

/*
 * In-place multiply by 10 and add a decimal digit:
 *
 *     *x = *x * 10 + digit,  0 <= digit <= 9
 *
 * The mantissa is treated as a plain 96-bit unsigned integer.
 * The exponent is not modified. The caller initializes the accumulator
 * to zero and normalizes it after digit collection.
 *
 * Returns 1 if the operation succeeds, 0 if the result would overflow
 * the 96-bit mantissa.
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
		if (midlo > 0x9999999999999990ULL)
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

/*
 * In-place multiply by 16 and add a hexadecimal digit:
 *
 *     *x = *x * 16 + digit,  0 <= digit <= 15
 *
 * The mantissa is treated as a plain 96-bit unsigned integer.
 * The exponent is not modified.
 *
 * Multiplication by 16 is a pure left shift by four bits. If the shift
 * overflows the 96-bit accumulator, the operation is rejected.
 * A nonzero discarded digit is retained as a sticky bit for subsequent
 * rounding; a zero digit must not set the sticky bit.
 *
 * Returns 1 if the digit was added successfully, 0 if the accumulator
 * is already full.
 */
int __LDL_mul_16_add(LDL *x, int digit)
{
	unsigned long hi = x->l.high;
	unsigned long mid = x->l.middle;
	unsigned long lo = x->l.low;

	/* Saturation: hi's top nibble non-zero means x16 overflows 96 bits. */
	if (hi >= 0x10000000UL) {
		/* All 96 bits are taken, and a dropped digit lies below them,
		 * far below the 64 that get rounded: only whether it is non-zero
		 * matters, as a sticky bit.  Zeros must not set it, or an exact
		 * tie rounds up. */
		if (digit)
			x->l.low |= 1;
		return 0;
	}

	x->l.high = (hi << 4) | (mid >> 28);
	x->l.middle = (mid << 4) | (lo >> 28);
	x->l.low = (lo << 4) | ((unsigned long) digit & 0xF);

	return 1;
}

/*
 * Normalize an integer accumulator built by __LDL_mul_10_add /
 * __LDL_mul_16_add.
 *
 * Shift the mantissa left until its highest set bit is at b95.
 * Set the exponent to 16383, making the normalized mantissa represent
 * a value in [1, 2).
 *
 * The caller is responsible for applying the exponent corresponding
 * to the decimal or hexadecimal scaling.
 *
 * A zero mantissa remains zero.
 *
 * Precondition: the accumulator has not yet been normalized.
 */
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

/* Convert an LDL value to IEEE-754 binary32.
 * sticky2 indicates whether additional discarded bits are nonzero.
 */
float
__LDL_to_float(const LDL *lv, unsigned int sticky2)
{
	union {
		float f;
		unsigned int u;
	} u;

	unsigned long long m;
	int e;
	int drop;

	m = ((unsigned long long) lv->l.high << 32)
			| lv->l.middle;

	e = (int) lv->l.exp - 16383;
	drop = 64 - 24;

	if (m == 0)
		return 0.0f;

	if (e < -126)
		drop += -126 - e;

	if (drop > 64) {
		m = 0;
	} else if (drop == 64) {
		if ((m << 1) != 0 || lv->l.low != 0 || sticky2 != 0) {
			m = 1ULL << 63;
			e++;
		} else {
			m = 0;
		}
	} else {
		unsigned long long half = 1ULL << (drop - 1);
		unsigned long long lost = m & ((half << 1) - 1);

		m -= lost;

		if ((lost & half)
				&& ((lost & (half - 1))
						|| lv->l.low != 0
						|| sticky2 != 0
						|| (m & (half << 1)))) {
			m += half << 1;

			if (m == 0) {
				m = 1ULL << 63;
				e++;
			}
		}
	}

	/* Convert the rounded mantissa and exponent to IEEE-754 float. */
	if (m == 0) {
		u.u = 0;
	} else if (e > 127) {
		u.u = 0x7f800000UL;
	} else if (e >= -126) {
		u.u = ((unsigned int) (e + 127) << 23)
				| ((unsigned int) (m >> 40) & 0x7fffffUL);
	} else {
		int shift = -126 - e;

		if (shift >= 24)
			u.u = 0;
		else
			u.u = (unsigned int) (m >> (40 + shift));
	}

	return u.f;
}

/* Convert an LDL value to IEEE-754 binary64.
 * sticky2 indicates whether additional discarded bits are nonzero.
 */
double
__LDL_to_double(const LDL *lv, unsigned int sticky2)
{
	union _d_bits u;
	unsigned long long m64;
	unsigned long long mant;
	unsigned long lo;
	int e, E;
	unsigned int guard;
	unsigned int sticky;

	m64 = ((unsigned long long) lv->l.high << 32)
			| (unsigned long long) lv->l.middle;
	lo = lv->l.low;

	e = (int) lv->l.exp - 16383;
	E = e + 1023;

	/* Zero. */
	if (m64 == 0 && lo == 0 && sticky2 == 0)
		return 0.0;

	/* Overflow and special exponent. */
	if (E >= 0x7ff)
			{
		u.u = 0;
		u.b.sign = 0;
		u.b.exp = 0x7ff;
		u.b.frac0 = 0;
		u.b.frac1 = 0;
		return u.d;
	}

	if (E >= 1)
			{
		/*
		 * Normal number:
		 * Keep 53 significant bits.
		 * Discard the lower 43 bits of the 96-bit mantissa.
		 */
		mant = m64 >> 11;

		guard = (m64 >> 10) & 1;
		sticky = ((m64 & 0x3ffULL) != 0)
				|| (lo != 0)
				|| (sticky2 != 0);

		/* Round to nearest, ties to even. */
		if (guard && (sticky || (mant & 1)))
				{
			++mant;

			/* Rounding overflow: renormalize. */
			if (mant == (1ULL << 53))
					{
				mant = 1ULL << 52;
				++E;
			}
		}

		/* Check overflow after rounding. */
		if (E >= 0x7ff)
				{
			u.u = 0;
			u.b.sign = 0;
			u.b.exp = 0x7ff;
			u.b.frac0 = 0;
			u.b.frac1 = 0;
			return u.d;
		}

		u.u = 0;
		u.b.sign = 0;
		u.b.exp = (unsigned int) E;
		u.b.frac0 = (unsigned int)
				((mant >> 32) & 0xfffffULL);
		u.b.frac1 = (unsigned int) mant;

		return u.d;
	}
	else
	{
		/*
		 * Subnormal number.
		 * Round directly from the complete 96-bit mantissa.
		 */
		int shift = 12 - E;

		if (shift < 64)
				{
			mant = m64 >> shift;
			guard = (m64 >> (shift - 1)) & 1;

			sticky = (lo != 0) || (sticky2 != 0);

			if (shift > 1)
				sticky |= (m64 & ((1ULL << (shift - 1)) - 1)) != 0;
		}
		else if (shift == 64)
				{
			mant = 0;
			guard = (m64 >> 63) & 1;
			sticky = (lo != 0) || (sticky2 != 0)
					|| ((m64 & 0x7fffffffffffffffULL) != 0);
		}
		else if (shift <= 96)
				{
			unsigned int s = (unsigned int) (shift - 64);

			mant = 0;
			guard = (lo >> (s - 1)) & 1;

			sticky = (sticky2 != 0);

			if (s > 1)
				sticky |= (lo & ((1UL << (s - 1)) - 1)) != 0;

			sticky |= (m64 != 0);
		}
		else
		{
			mant = 0;
			guard = 0;
			sticky = (m64 != 0) || (lo != 0) || (sticky2 != 0);
		}

		if (guard && (sticky || (mant & 1)))
			++mant;
	}

	/*
	 * For subnormal results, mant contains the complete 52-bit
	 * fraction. Rounding may produce 1 << 52, which is the
	 * smallest normal double.
	 */
	u.u = 0;
	u.b.sign = 0;
	u.b.exp = (unsigned int) (mant >> 52);
	u.b.frac0 = (unsigned int)
			((mant >> 32) & 0xfffffULL);
	u.b.frac1 = (unsigned int) mant;

	return u.d;
}

/* Convert an LDL value to Motorola 80-bit extended precision.
 * sticky2 indicates whether additional discarded bits are nonzero.
 */
long double
__LDL_to_long_double(const LDL *lv, unsigned int sticky2)
{
	union _ld_bits u;
	unsigned long long m64;
	unsigned long long mant;
	unsigned long lo;
	int E;
	unsigned int guard;
	unsigned int sticky;

	m64 = ((unsigned long long) lv->l.high << 32)
			| (unsigned long long) lv->l.middle;
	lo = lv->l.low;
	E = (int) lv->l.exp;

	u.ld = 0.0L;

	/* Handle overflow. */
	if (E >= 0x7fff)
			{
		u.b.sign = 0;
		u.b.pad = 0;
		u.b.exp = 0x7fff;
		u.b.mant = 0x8000000000000000ULL;
		return u.ld;
	}

	/* Zero. */
	if (m64 == 0 && lo == 0)
		return u.ld;

	if (E >= 1)
			{
		/*
		 * Normal number:
		 * Keep the upper 64 bits and round using the lower 32 bits.
		 */
		mant = m64;
		guard = (lo >> 31) & 1;
		sticky = (lo & 0x7fffffffUL) != 0 || (sticky2 != 0);

		if (guard && (sticky || (mant & 1)))
				{
			++mant;

			/* Rounding overflow: renormalize. */
			if (mant == 0)
					{
				mant = 0x8000000000000000ULL;
				++E;
			}
		}
	}
	else if (E >= -62)
			{
		/*
		 * Subnormal number:
		 * Combine the exponent adjustment and precision reduction
		 * before rounding. This avoids double rounding.
		 */
		unsigned int shift = (unsigned int) (-E);

		mant = m64 >> shift;
		guard = (m64 >> (shift - 1)) & 1;

		sticky = (lo != 0) || (sticky2 != 0);

		if (shift > 1)
			sticky |= (m64 & ((1ULL << (shift - 1)) - 1)) != 0;

		if (guard && (sticky || (mant & 1)))
			++mant;

		/*
		 * Rounding may turn the largest subnormal into the
		 * smallest normal number. Keep the integer bit in that case.
		 */
		E = (mant & 0x8000000000000000ULL) ? 1 : 0;
	}
	else if (E == -63)
			{
		mant = m64 >> 63;

		/* Remaining bits are below the smallest subnormal. */
		guard = 0;
		sticky = 0;

		E = 0;
	}
	else
	{
		/* Too small to round to a nonzero subnormal. */
		mant = 0;
		E = 0;
	}

	/* Store the Motorola extended-precision representation. */
	u.b.sign = 0;
	u.b.pad = 0;
	u.b.exp = (unsigned int) E;
	u.b.mant = mant;

	return u.ld;
}
