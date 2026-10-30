/* ==========================================================================
 *  ami_scanf_math.c
 *
 *  The shared scanf FP math core.
 *
 *  See ami_printf_math.h for the contract of each function.  All arithmetic
 *  here is signless, on the local LDL view: 16-bit exponent, 96-bit mantissa
 *  storage with the integer bit at b95.
 *
 *  Known limitation: __LDL_mul currently produces only 80 bits of mantissa
 *  (5 x 16-bit limbs); the low 16 bits of the result are zeroed.  The
 *  pipeline carries 96 bits through __LDL_mul_10 and __LDL_mul_10_extract,
 *  but the one real multiplication per pow10_ld step discards the bottom
 *  16 bits.  This is the "80-bit multiplication, 96-bit pipeline" design:
 *  enough headroom for the extraction chain, at the cost of a slightly
 *  larger residual in the last digit than a true 96-bit multiply would give.
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
 * The mantissa is treated as a plain 96-bit unsigned integer.  The
 * exponent is NOT maintained by this routine; the caller starts with
 * *x = { 0, 0, 0, 0 } and calls __LDL_normalize once at the end to
 * find the highest set bit and set the exponent.
 *
 * If the 96-bit mantissa overflows (carry out of bit 95), the top
 * bits are lost.  The caller is responsible for detecting saturation
 * (e.g. by checking that *x->l.high still fits, or by counting digits
 * and bailing out past ~28) if that matters.
 */
void __LDL_mul_10_add(LDL *x, int digit)
{
    unsigned long hi = x->l.high, mid = x->l.middle, lo = x->l.low;
    unsigned long a_hi, a_mid, a_lo;
    unsigned long b_hi, b_mid, b_lo;
    unsigned long res_lo, res_mid, res_hi, carry;

    /* x * 10 = (x << 3) + (x << 1) = 8x + 2x.
     * This is exact (no truncation): the only loss is the carry past
     * bit 95, which is the caller's saturation concern. */

    /* a = x << 3 */
    a_hi  = (hi  << 3) | (mid >> 29);
    a_mid = (mid << 3) | (lo  >> 29);
    a_lo  = (lo  << 3);

    /* b = x << 1 */
    b_hi  = (hi  << 1) | (mid >> 31);
    b_mid = (mid << 1) | (lo  >> 31);
    b_lo  = (lo  << 1);

    /* res = a + b */
    res_lo = a_lo + b_lo;
    carry  = (res_lo < a_lo);
    res_mid = a_mid + b_mid + carry;
    carry   = (res_mid < a_mid) || (res_mid < b_mid);
    if (carry > 1)
        carry = 1;
    res_hi = a_hi + b_hi + carry;

    /* + digit */
    res_lo += (unsigned long)digit;
    if (res_lo < (unsigned long)digit) {
        res_mid++;
        if (res_mid == 0)
            res_hi++;
    }

    x->l.high   = res_hi;
    x->l.middle = res_mid;
    x->l.low    = res_lo;
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
 * bit 95, which is the caller's saturation concern.
 */
void __LDL_mul_16_add(LDL *x, int digit)
{
	unsigned long hi = x->l.high, mid = x->l.middle, lo = x->l.low;
	unsigned long res_hi, res_mid, res_lo;

	/* x * 16 = x << 4 */
	res_hi  = (hi  << 4) | (mid >> 28);
	res_mid = (mid << 4) | (lo  >> 28);
	res_lo  = (lo  << 4);

	/* + digit */
	res_lo += (unsigned long)digit;
	if (res_lo < (unsigned long)digit) {
		res_mid++;
		if (res_mid == 0)
			res_hi++;
	}

	x->l.high   = res_hi;
	x->l.middle = res_mid;
	x->l.low    = res_lo;
	/* x->l.exp unchanged (0 from the caller's initializer) */
}

/* Normalize an LDL built by __LDL_mul_10_add (which leaves exp = 0):
 * find the highest set bit b in the 96-bit mantissa, shift left so it
 * lands at b95, and set exp = 16383 + b.  A zero mantissa stays zero.
 *
 * Precondition: x->l.exp == 0 (the accumulator's initial state).  If
 * x is already normalized (high != 0, bit 31 set), this is a no-op
 * except for setting exp. */
void __LDL_normalize(LDL *x)
{
	unsigned long hi = x->l.high, mid = x->l.middle, lo = x->l.low;
	int n, b;

	if (hi == 0 && mid == 0 && lo == 0) {
		x->l.exp = 0;
		return;
	}

	if (hi != 0) {
		n = __builtin_clz(hi);      /* 0..31 */
		b = 95 - n;
		if (n > 0) {
			x->l.high   = (hi  << n) | (mid >> (32 - n));
			x->l.middle = (mid << n) | (lo  >> (32 - n));
			x->l.low    = (lo  << n);
		}
	} else if (mid != 0) {
		n = __builtin_clz(mid);     /* 0..31 */
		b = 63 - n;
		if (n > 0) {
			x->l.high   = (mid << n) | (lo >> (32 - n));
			x->l.middle = (lo  << n);
			x->l.low    = 0;
		}
	} else {
		n = __builtin_clz(lo);      /* 0..31 */
		b = 31 - n;
		if (n > 0) {
			x->l.high   = (lo << n);
			x->l.middle = 0;
			x->l.low    = 0;
		}
	}

	x->l.exp = (unsigned short)(16383 + b);
}


double __ldl_to_double(const LDL *lv)
{
	unsigned long hi, mid, lo;
	unsigned long long mant;
	int e, E;

	hi  = lv->l.high;
	mid = lv->l.middle;
	lo  = lv->l.low;

	if (hi == 0)
		return 0.0;

	/* Unbiased binary exponent of the value: value = M * 2^(e - 95). */
	e = (int)lv->l.exp - 16383;

	/* Form the 96-bit mantissa as a 64-bit + 32-bit pair.  We only need
	 * the top 54 bits (53 significant + 1 guard) for round-to-nearest-
	 * even at binary64 precision.  The top 64 bits of the mantissa are
	 * hi:mid, and lo provides the sticky bits. */
	mant = ((unsigned long long)hi << 32) | mid;   /* top 64 bits */

	/* The value's integer bit is at bit 63 of mant (for a normalized
	 * LDL).  We want to keep 53 bits, i.e. shift right by 63 - 52 = 11
	 * to put the integer bit at bit 52 of the 53-bit result. */
	{
		unsigned long long m53 = mant >> 11;          /* top 53 bits */
		unsigned long long guard = (mant >> 10) & 1;  /* bit 10 = next */
		unsigned long long sticky = (mant & 0x3FFULL) != 0 || lo != 0;

		/* Round to nearest-even. */
		if (guard && (sticky || (m53 & 1))) {
			m53++;
			if (m53 >= (1ULL << 53)) {
				m53 >>= 1;   /* mantissa overflowed: renormalize */
				e++;
			}
		}

		/* m53 now holds 1.f52 in the top 53 bits (bit 52 = integer bit).
		 * Extract the 52-bit fraction. */
		E = e + 1023;
		if (E >= 0x7FF) {
			/* Overflow: return +inf.  Caller applies sign. */
			return (double)(1.0 / 0.0);
		}
		if (E <= 0) {
			/* Underflow: no denormals needed for scanf's typical
			 * inputs, but handle gracefully by returning 0. */
			return 0.0;
		}

		{
			unsigned long long frac52 = m53 & 0xFFFFFFFFFFFFFULL;
			union _d_bits u;
			u.u = 0;
			u.b.sign  = 0;
			u.b.exp   = (unsigned)E;
			u.b.frac0 = (unsigned)(frac52 >> 32);
			u.b.frac1 = (unsigned)(frac52 & 0xFFFFFFFFUL);
			return u.d;
		}
	}
}

long double __ldl_to_long_double(const LDL *lv)
{
	union _ld_bits u;
	unsigned long hi, mid, lo;
	int e;

	hi  = lv->l.high;
	mid = lv->l.middle;
	lo  = lv->l.low;

	if (hi == 0 && mid == 0 && lo == 0)
		return 0.0L;

	e = (int)lv->l.exp - 16383;

	/* Round the 96-bit mantissa to 64 bits.  For a normalized LDL, the
	 * top 64 bits (hi:mid) already have the integer bit at b63, so no
	 * shift is needed; lo provides the guard and sticky bits.
	 *
	 * Wait -- the integer bit at b63 of hi:mid corresponds to b95 of
	 * the 96-bit mantissa, and the LDL's exponent is already scaled
	 * such that the value is 1.f63 * 2^e.  So hi:mid *is* the 64-bit
	 * mantissa with the integer bit at b63.  We just round lo into it. */
	if (lo != 0) {
		/* Guard bit is bit 31 of lo (which would become bit -1 of
		 * the 64-bit mantissa after truncation).  Sticky is the rest
		 * of lo. */
		unsigned long guard = (lo >> 31) & 1;
		unsigned long sticky = (lo & 0x7FFFFFFFUL) != 0;

		if (guard && (sticky || (mid & 1))) {
			/* Increment the 64-bit mantissa hi:mid by 1. */
			mid++;
			if (mid == 0) {
				hi++;
				if (hi == 0) {
					/* Mantissa overflowed 64 bits: renormalize. */
					hi = 0x80000000UL;
					e++;
				}
			}
		}
	}

	u.ld = 0.0L;
	u.b.sign = 0;
	u.b.exp  = (unsigned short)(e + 16383);
	u.b.pad  = 0;
	u.b.mant = ((unsigned long long)hi << 32) | mid;
	return u.ld;
}
