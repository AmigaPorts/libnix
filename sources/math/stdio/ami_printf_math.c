/* ==========================================================================
 *  ami_printf_math.c
 *
 *  Out-of-line definitions for the shared printf FP math core.
 *
 *  See ami_printf_math.h for the contract of each function.  Everything
 *  here is signless 80-bit arithmetic on the local LDL view; sign handling
 *  is done by the callers.
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

/* 0.1 = 1.6 * 2^-4, biased exponent 16383-4, mantissa 0xCCCCCCCCCCCCCCCC. */
const LDL WORK_0_1 = {
	.l = { 16383 + (-4), 0, 0xCCCCCCCCUL, 0xCCCCCCCCUL }
};

LDL __LDL_from_double(double d)
{
	union _d_bits u;
	LDL r;
	unsigned long long mant;
	int exp;

	u.d = d;

	/* Exact zero */
	if (u.b.exp == 0 && u.b.frac0 == 0 && u.b.frac1 == 0) {
		r.l.exp = 0;
		r.l.pad = 0;
		r.l.middle = 0;
		r.l.lower = 0;
		return r;
	}

	if (u.b.exp == 0) {
		/* Subnormal double: exponent = -1022, no hidden bit */
		mant = ((unsigned long long) u.b.frac0 << 32) | u.b.frac1;
		exp = -1022;

		/* Normalize until bit 52 becomes 1 */
		while ((mant >> 52) == 0) {
			mant <<= 1;
			exp--;
		}
	} else {
		/* Normal double: add hidden bit */
		mant = (1ULL << 52) | ((unsigned long long) u.b.frac0 << 32) | u.b.frac1;
		exp = (int) u.b.exp - 1023;
	}

	/* Reposition the 53-bit mantissa so the integer bit lands at b63. */
	{
		unsigned long long mant64 = mant << (63 - 52);
		r.l.exp = (unsigned short) (exp + 16383);
		r.l.pad = 0;
		r.l.middle = (unsigned long) (mant64 >> 32);
		r.l.lower = (unsigned long) (mant64 & 0xFFFFFFFFUL);
	}
	return r;
}

LDL __LDL_from_long_double(long double ld)
{
	union _ld_bits u;
	LDL r;
	unsigned short E;
	unsigned long long mant;

	u.ld = ld;
	E = u.b.exp;
	mant = u.b.mant;

	/* Exact zero */
	if (E == 0 && mant == 0) {
		r.l.exp = 0;
		r.l.pad = 0;
		r.l.middle = 0;
		r.l.lower = 0;
		return r;
	}

	if (E == 0) {
		/* Subnormal long double: normalize mantissa */
		int exp = -16382;
		while ((mant & 0x8000000000000000ULL) == 0) {
			mant <<= 1;
			exp--;
		}
		r.l.exp = (unsigned short) (exp + 16383);
		r.l.pad = 0;
		r.l.middle = (unsigned long) (mant >> 32);
		r.l.lower = (unsigned long) (mant & 0xFFFFFFFFUL);
		return r;
	}

	/* Normal long double: copy fields directly */
	r.l.exp = E;
	r.l.pad = 0;
	r.l.middle = (unsigned long) (mant >> 32);
	r.l.lower = (unsigned long) (mant & 0xFFFFFFFFUL);
	return r;
}

#ifndef __mc68881__
LDL __LDL_mul(LDL a, LDL b)
{
	int exp_a, exp_b, exp_r;
	unsigned long long hi, lo;
	unsigned long long mant_r;
	unsigned int guard = 0, sticky = 0;
	unsigned int a_lo, a_hi, b_lo, b_hi;
	unsigned long long p0, p1, p2, p3, mid;
	LDL r;

	/* Zero */
	if (a.l.middle == 0)
		return a;
	if (b.l.middle == 0)
		return b;

	exp_a = a.l.exp - 16383;
	exp_b = b.l.exp - 16383;
	exp_r = exp_a + exp_b;

	a_lo = a.l.lower;
	a_hi = a.l.middle;
	b_lo = b.l.lower;
	b_hi = b.l.middle;

	p0 = (unsigned long long) a_lo * b_lo;
	p1 = (unsigned long long) a_lo * b_hi;
	p2 = (unsigned long long) a_hi * b_lo;
	p3 = (unsigned long long) a_hi * b_hi;

	mid = (p0 >> 32) + (p1 & 0xFFFFFFFFULL) + (p2 & 0xFFFFFFFFULL);

	hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
	lo = (mid << 32) | (p0 & 0xFFFFFFFFULL);

	/* Normalize product so that mant_r has integer bit at 63 */
	if (hi & 0x8000000000000000ULL) {
		mant_r = hi;
		guard  = (unsigned int) (lo >> 63);
		sticky = (lo & 0x7FFFFFFFFFFFFFFFULL) != 0;
		exp_r++;
	} else {
		unsigned long long lo_shifted;
		mant_r = (hi << 1) | (lo >> 63);
		lo_shifted = lo << 1;
		guard  = (unsigned int) (lo_shifted >> 63);
		sticky = (lo_shifted & 0x7FFFFFFFFFFFFFFFULL) != 0;
	}

	/* Round to nearest even */
	if (guard && (sticky || (mant_r & 1ULL))) {
		mant_r++;
		if (mant_r == 0) {
			mant_r = 0x8000000000000000ULL;
			exp_r++;
		}
	}

	r.l.exp    = exp_r + 16383;
	r.l.pad    = 0;
	r.l.middle = (unsigned long) (mant_r >> 32);
	r.l.lower  = (unsigned long) mant_r;
	return r;
}
#endif /* !__mc68881__ */

LDL __LDL_mul_10(LDL x)
{
	LDL r;
	unsigned long hi, lo;
	unsigned long hi2, lo2;
	unsigned long res_lo, res_hi, carry;

	/* Zero check */
	if (x.l.middle == 0 && x.l.lower == 0)
		return x;

	r = x;

	hi = x.l.middle;
	lo = x.l.lower;

	/* 64-bit right shift by 2 (x / 4) */
	hi2 = hi >> 2;
	lo2 = (lo >> 2) | (hi << 30);

	/* 64-bit addition: (x + x/4) */
	res_lo = lo + lo2;
	carry  = (res_lo < lo);
	res_hi = hi + hi2 + carry;

	/* Check 64-bit overflow (bit 63 set) */
	if (res_hi < hi || (res_hi == hi && hi2 > 0)) {
		/* Shift right 1 bit to re-normalize */
		res_lo = (res_lo >> 1) | (res_hi << 31);
		res_hi = (res_hi >> 1) | 0x80000000;
		r.l.exp += 1;
	}

	/* Scale by 8 (add 3 to exponent) */
	r.l.exp += 3;

	r.l.middle = res_hi;
	r.l.lower  = res_lo;

	return r;
}

int __LDL_mul_10_extract(LDL *x)
{
	int exp_raw = x->l.exp;
	unsigned long hi = x->l.middle;
	unsigned long lo = x->l.lower;
	int exp = exp_raw - 16383;
	unsigned long z = 0;

	if (exp >= 0) {
		/* 1. Extract the leading digit */
		z = hi >> (31 - exp);

		/* 2. Subtract the digit from the upper mantissa */
		hi -= (z << (31 - exp));

		/* 3. In-place normalization */
		if (!hi) {
			if (!lo) {
				/* Exact integer without a fraction -> value becomes clean zero */
				x->l.exp = 0;
				return (int) z;
			}
			{
				/* 'middle' is completely empty, pull bits up from 'lower' */
				int n = __builtin_clz(lo);
				hi = lo << n;
				exp_raw = exp + 16383 - 32 - n;
			}
		} else {
			/* 'middle' still has bits but gained leading zeros */
			int n = __builtin_clz(hi);
			unsigned long long combined =
			    ((unsigned long long) hi << 32) | lo;
			combined <<= n;
			hi = (unsigned long) (combined >> 32);
			lo = (unsigned long) combined;
			exp_raw = exp + 16383 - n;
		}
	}

	/* 4. Fast inlined multiply-by-10: x * 10 = (x + x/4) * 8 */
	{
		unsigned long hi2 = hi >> 2;
		unsigned long lo2 = (lo >> 2) | (hi << 30);
		unsigned long res_lo = lo + lo2;
		unsigned long carry  = (res_lo < lo);
		unsigned long res_hi = hi + hi2 + carry;

		if (res_hi < hi || (res_hi == hi && hi2 > 0)) {
			res_lo = (res_lo >> 1) | (res_hi << 31);
			res_hi = (res_hi >> 1) | 0x80000000UL;
			exp_raw += 1;
		}

		x->l.exp    = (unsigned short) (exp_raw + 3);
		x->l.middle = res_hi;
		x->l.lower  = res_lo;
	}

	return (int) z;
}

LDL pow10_ld(LDL result, int n)
{
	/* 10^(2^bit) */
	static const LDL pow10_pos_table[] = {
	    { .l = { 16383 + 3,     0, 0xA0000000UL, 0x00000000UL } },
	    { .l = { 16383 + 6,     0, 0xC8000000UL, 0x00000000UL } },
	    { .l = { 16383 + 13,    0, 0x9C400000UL, 0x00000000UL } },
	    { .l = { 16383 + 26,    0, 0xBEBC2000UL, 0x00000000UL } },
	    { .l = { 16383 + 53,    0, 0x8E1BC9BFUL, 0x04000000UL } },
	    { .l = { 16383 + 106,   0, 0x9DC5ADA8UL, 0x2B70B59DUL } },
	    { .l = { 16383 + 212,   0, 0xC2781F49UL, 0xFFCFA6D5UL } },
	    { .l = { 16383 + 425,   0, 0x93BA47C9UL, 0x80E98CDFUL } },
	    { .l = { 16383 + 850,   0, 0xAA7EEBFBUL, 0x9DF9DE8DUL } },
	    { .l = { 16383 + 1700,  0, 0xE319A0AEUL, 0xA60E91C6UL } },
	    { .l = { 16383 + 3401,  0, 0xC9767586UL, 0x81750C17UL } },
	    { .l = { 16383 + 6803,  0, 0x9E8B3B5DUL, 0xC53D5DE4UL } },
	    { .l = { 16383 + 13606, 0, 0xC4605202UL, 0x8A20979AUL } }
	};

	/* 1 / 10^(2^bit) */
	static const LDL pow10_neg_table[] = {
	    { .l = { 16383 + (-4),     0, 0xCCCCCCCCUL, 0xCCCCCCCDUL } },
	    { .l = { 16383 + (-7),     0, 0xA3D70A3DUL, 0x70A3D70BUL } },
	    { .l = { 16383 + (-14),    0, 0xD1B71758UL, 0xE219652CUL } },
	    { .l = { 16383 + (-27),    0, 0xABCC7711UL, 0x8461CEFDUL } },
	    { .l = { 16383 + (-54),    0, 0xE69594BEUL, 0xC44DE15CUL } },
	    { .l = { 16383 + (-107),   0, 0xCFB11EADUL, 0x453994BBUL } },
	    { .l = { 16383 + (-213),   0, 0xA87FEA27UL, 0xA539E9A6UL } },
	    { .l = { 16383 + (-426),   0, 0xDDD0467CUL, 0x64BCE4A1UL } },
	    { .l = { 16383 + (-851),   0, 0xC0314325UL, 0x637A193AUL } },
	    { .l = { 16383 + (-1701),  0, 0x9049EE32UL, 0xDB23D21DUL } },
	    { .l = { 16383 + (-3402),  0, 0xA2A682A5UL, 0xDA57C0BFUL } },
	    { .l = { 16383 + (-6804),  0, 0xCEAE534FUL, 0x34362DE5UL } },
	    { .l = { 16383 + (-13607), 0, 0xA6DD04C8UL, 0xD2CE9FDFUL } }
	};

	const LDL *table;
	unsigned int exp;
	unsigned int bit;

	if (n == 0)
		return result;

	table = n < 0 ? pow10_neg_table : pow10_pos_table;
	exp = (unsigned int) (n < 0 ? -n : n);

	for (bit = 0; exp; ++bit, exp >>= 1) {
		if (exp & 1) {
			result = __LDL_mul(result, table[bit]);
		}
	}

	return result;
}
