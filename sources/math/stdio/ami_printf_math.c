/* ==========================================================================
 *  ami_printf_math.c
 *
 *  The shared printf FP math core.
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
#include "ami_printf_math.h"

/* 0.1 = 1.6 * 2^-4, biased exponent 16383-4.
 * 96-bit mantissa 0xCCCCCCCC_CCCCCCCC_CCCCCCCD (round-to-nearest, matching
 * pow10_neg_table[0]). */
const LDL WORK_0_1 = {
	.l = { 16383 + (-4), 0xCCCCCCCCUL, 0xCCCCCCCCUL, 0xCCCCCCCDUL }
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
		r.l.high = 0;
		r.l.middle = 0;
		r.l.low = 0;
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

	/* Reposition the 53-bit mantissa so the integer bit lands at b63 of the
	 * 64-bit hardware view, which is b95 of the 96-bit LDL view.  The low
	 * 32 bits of the LDL mantissa stay zero because a double only carries
	 * 53 significant bits. */
	{
		unsigned long long mant64 = mant << (63 - 52);
		r.l.exp = (unsigned short) (exp + 16383);
		r.l.high = (unsigned long) (mant64 >> 32);
		r.l.middle = (unsigned long) (mant64 & 0xFFFFFFFFUL);
		r.l.low = 0UL;
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
		r.l.high = 0;
		r.l.middle = 0;
		r.l.low = 0;
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
		r.l.high = (unsigned long) (mant >> 32);
		r.l.middle = (unsigned long) (mant & 0xFFFFFFFFUL);
		r.l.low = 0UL;
		return r;
	}

	/* Normal long double: copy fields directly.  The low 32 bits of the
	 * 96-bit LDL are zero because the native mantissa is 64 bits. */
	r.l.exp = E;
	r.l.high = (unsigned long) (mant >> 32);
	r.l.middle = (unsigned long) (mant & 0xFFFFFFFFUL);
	r.l.low = 0UL;
	return r;
}

/*
 * 80-bit mantissa multiplication using 16x16 -> 32 bit partial products.
 *
 * Inputs are 96-bit LDL values, but only the top 80 bits (5 x 16-bit limbs,
 * s.word[0..4]) participate.  The result is 80 bits, written back to
 * s.word[0..4]; s.word[5] (the lowest 16 bits of the 96-bit storage) is
 * zeroed so that no stale value leaks into a later multiply.
 *
 * Normalized inputs have the integer bit (s.word[0] bit 15) set, so the
 * product lies in [2^158, 2^160); the normalization step below shifts once
 * to bring the integer bit back to bit 79 (s.word[0] bit 15).
 */
LDL __LDL_mul(LDL a, LDL b)
{
	int exp_a, exp_b, exp_r;
	unsigned short a0, a1, a2, a3, a4, b0, b1, b2, b3, b4;
	unsigned int p00, p01, p02, p03, p04,
	             p10, p11, p12, p13, p14,
	             p20, p21, p22, p23, p24,
	             p30, p31, p32, p33, p34,
	             p40, p41, p42, p43, p44;
	unsigned int s, c;
	unsigned int r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;  /* 160-bit product */
	unsigned short m0, m1, m2, m3, m4;                      /* 80-bit result */
	unsigned int guard = 0, sticky = 0;
	LDL r;

	/* Zero: for normalized inputs the high word is nonzero. */
	if (a.l.high == 0 && a.l.middle == 0 && a.l.low == 0)
		return a;
	if (b.l.high == 0 && b.l.middle == 0 && b.l.low == 0)
		return b;

	exp_a = a.l.exp - 16383;
	exp_b = b.l.exp - 16383;
	exp_r = exp_a + exp_b;

	/* 80-bit mantissa as 5 limbs.  a4 holds the integer bit (bit 79). */
	a0 = a.s.word[4];
	a1 = a.s.word[3];
	a2 = a.s.word[2];
	a3 = a.s.word[1];
	a4 = a.s.word[0];
	b0 = b.s.word[4];
	b1 = b.s.word[3];
	b2 = b.s.word[2];
	b3 = b.s.word[1];
	b4 = b.s.word[0];

	/* 5x5 = 25 partial products (16x16 -> 32).  p_ij has weight 2^(16(i+j)). */
	p00 = (unsigned long) a0 * b0;
	p01 = (unsigned long) a0 * b1;
	p10 = (unsigned long) a1 * b0;
	p02 = (unsigned long) a0 * b2;
	p11 = (unsigned long) a1 * b1;
	p20 = (unsigned long) a2 * b0;
	p03 = (unsigned long) a0 * b3;
	p12 = (unsigned long) a1 * b2;
	p21 = (unsigned long) a2 * b1;
	p30 = (unsigned long) a3 * b0;
	p04 = (unsigned long) a0 * b4;
	p13 = (unsigned long) a1 * b3;
	p22 = (unsigned long) a2 * b2;
	p31 = (unsigned long) a3 * b1;
	p40 = (unsigned long) a4 * b0;
	p14 = (unsigned long) a1 * b4;
	p23 = (unsigned long) a2 * b3;
	p32 = (unsigned long) a3 * b2;
	p41 = (unsigned long) a4 * b1;
	p24 = (unsigned long) a2 * b4;
	p33 = (unsigned long) a3 * b3;
	p42 = (unsigned long) a4 * b2;
	p34 = (unsigned long) a3 * b4;
	p43 = (unsigned long) a4 * b3;
	p44 = (unsigned long) a4 * b4;

	/* Column-wise accumulation into r0..r9, each a 16-bit limb. */
	c = 0;

	s  = c + (p00 & 0xFFFFu);
	r0 = s & 0xFFFFu;  c = s >> 16;

	s  = c + (p01 & 0xFFFFu) + (p10 & 0xFFFFu) + (p00 >> 16);
	r1 = s & 0xFFFFu;  c = s >> 16;

	s  = c + (p02 & 0xFFFFu) + (p11 & 0xFFFFu) + (p20 & 0xFFFFu)
	   + (p01 >> 16) + (p10 >> 16);
	r2 = s & 0xFFFFu;  c = s >> 16;

	s  = c + (p03 & 0xFFFFu) + (p12 & 0xFFFFu) + (p21 & 0xFFFFu) + (p30 & 0xFFFFu)
	   + (p02 >> 16) + (p11 >> 16) + (p20 >> 16);
	r3 = s & 0xFFFFu;  c = s >> 16;

	s  = c + (p04 & 0xFFFFu) + (p13 & 0xFFFFu) + (p22 & 0xFFFFu) + (p31 & 0xFFFFu) + (p40 & 0xFFFFu)
	   + (p03 >> 16) + (p12 >> 16) + (p21 >> 16) + (p30 >> 16);
	r4 = s & 0xFFFFu;  c = s >> 16;

	s  = c + (p14 & 0xFFFFu) + (p23 & 0xFFFFu) + (p32 & 0xFFFFu) + (p41 & 0xFFFFu)
	   + (p04 >> 16) + (p13 >> 16) + (p22 >> 16) + (p31 >> 16) + (p40 >> 16);
	r5 = s & 0xFFFFu;  c = s >> 16;

	s  = c + (p24 & 0xFFFFu) + (p33 & 0xFFFFu) + (p42 & 0xFFFFu)
	   + (p14 >> 16) + (p23 >> 16) + (p32 >> 16) + (p41 >> 16);
	r6 = s & 0xFFFFu;  c = s >> 16;

	s  = c + (p34 & 0xFFFFu) + (p43 & 0xFFFFu)
	   + (p24 >> 16) + (p33 >> 16) + (p42 >> 16);
	r7 = s & 0xFFFFu;  c = s >> 16;

	s  = c + (p44 & 0xFFFFu) + (p34 >> 16) + (p43 >> 16);
	r8 = s & 0xFFFFu;  c = s >> 16;

	r9 = c + (p44 >> 16);   /* < 2^16, product < 2^160 */

	/* Normalize: bring the integer bit back to bit 79 (s.word[0] bit 15). */
	if (r9 & 0x8000u) {
		m4 = (unsigned short) r9;
		m3 = (unsigned short) r8;
		m2 = (unsigned short) r7;
		m1 = (unsigned short) r6;
		m0 = (unsigned short) r5;
		guard  = (unsigned int) (r4 >> 15);
		sticky = ((r4 & 0x7FFFu) | r3 | r2 | r1 | r0) != 0;
		exp_r++;
	} else {
		m4 = (unsigned short) ((r9 << 1) | (r8 >> 15));
		m3 = (unsigned short) ((r8 << 1) | (r7 >> 15));
		m2 = (unsigned short) ((r7 << 1) | (r6 >> 15));
		m1 = (unsigned short) ((r6 << 1) | (r5 >> 15));
		m0 = (unsigned short) ((r5 << 1) | (r4 >> 15));
		guard  = (unsigned int) ((r4 << 1) >> 15);
		sticky = (((r4 << 1) & 0x7FFFu) | r3 | r2 | r1 | r0) != 0;
	}

	/* Round to nearest even. */
	if (guard && (sticky || (m0 & 1u))) {
		if (++m0 == 0) {
			if (++m1 == 0) {
				if (++m2 == 0) {
					if (++m3 == 0) {
						if (++m4 == 0) {
							m4 = 0x8000u;   /* 2^80 -> renormalize */
							exp_r++;
						}
					}
				}
			}
		}
	}

	r.l.exp     = (unsigned short) (exp_r + 16383);
	r.s.word[0] = m4;   /* bits 79..64 */
	r.s.word[1] = m3;   /* bits 63..48 */
	r.s.word[2] = m2;   /* bits 47..32 */
	r.s.word[3] = m1;   /* bits 31..16 */
	r.s.word[4] = m0;   /* bits 15..0  */
	r.s.word[5] = 0;    /* bits 95..80: not computed by the 80-bit multiply */
	return r;
}

void __LDL_mul_10(LDL * x)
{
	LDL r;
	unsigned long hi, mid, lo;
	unsigned long hi2, mid2, lo2;
	unsigned long res_lo, res_mid, res_hi, carry;

	/* Zero check across the full 96-bit storage. */
	if (x->l.high == 0)
		return;

	hi  = x->l.high;
	mid = x->l.middle;
	lo  = x->l.low;

	/* 96-bit right shift by 2 (x / 4). */
	hi2  = hi >> 2;
	mid2 = (mid >> 2) | (hi << 30);
	lo2  = (lo >> 2)  | (mid << 30);

	/* 96-bit cascade addition: x + x/4. */
	res_lo = lo + lo2;
	carry  = (res_lo < lo);

	res_mid = mid + mid2 + carry;
	carry   = (res_mid < mid) || (res_mid == mid && carry);

	res_hi = hi + hi2 + carry;

	/* Renormalize if bit 95 overflowed. */
	if (res_hi < hi || (res_hi == hi && hi2 > 0)) {
		res_lo  = (res_lo >> 1)  | (res_mid << 31);
		res_mid = (res_mid >> 1) | (res_hi << 31);
		res_hi  = (res_hi >> 1)  | 0x80000000UL;
		x->l.exp += 1;
	}

	/* Scale by 8 (add 3 to exponent). */
	x->l.exp += 3;

	x->l.high   = res_hi;
	x->l.middle = res_mid;
	x->l.low    = res_lo;
}

int __LDL_mul_10_extract(LDL *x)
{
	int exp_raw = x->l.exp;
	unsigned long hi  = x->l.high;
	unsigned long mid = x->l.middle;
	unsigned long lo  = x->l.low;
	int exp = exp_raw - 16383;
	unsigned long z = 0;

	if (exp >= 0) {
		/* 1. Extract the leading digit. */
		z = hi >> (31 - exp);

		/* 2. Subtract the digit from the upper mantissa. */
		hi -= (z << (31 - exp));
		/* 3. In-place normalization. */
		if (!hi) {
			if (!mid && !lo) {
				/* Exact integer without a fraction -> clean zero. */
				x->l.exp = 0;
				return (int) z;
			}
			if (!mid) {
				/* hi and mid are both zero; pull the low 32 bits up. */
				int n = __builtin_clz(lo);
				hi = lo << n;
				mid = 0UL;
				lo = 0UL;
				exp_raw = exp + 16383 - 64 - n;
			} else {
				/* hi is zero; pull mid|lo up into hi|mid. */
				int n = __builtin_clz(mid);
				unsigned long long combined = ((unsigned long long) mid << 32) | lo;
				combined <<= n;
				hi = (unsigned long) (combined >> 32);
				mid = (unsigned long) combined;
				lo = 0UL;
				exp_raw = exp + 16383 - 32 - n;
			}
		} else {
			int n = __builtin_clz(hi);
			hi  = (hi << n)  | (mid >> (32 - n));
			mid = (mid << n) | (lo >> (32 - n));
			lo  = (lo << n);
			exp_raw = exp + 16383 - n;
		}
	} else {
		/* exp < 0: value is < 1.  The mantissa has up to 4 bits of
		 * headroom above hi's bit 31 (from the ×10 shift).  Shift
		 * right by 4 to renormalize, and bump exp_raw by 4. */
		lo  = (lo >> 4)  | (mid << 28);
		mid = (mid >> 4) | (hi << 28);
		hi  = hi >> 4;
		exp_raw += 4;
	}
	/* 4. Inlined multiply-by-10: x * 10 = (x + x/4) * 8. */
	{
		unsigned long lo2  = (lo >> 2)  | (mid << 30);
		unsigned long mid2 = (mid >> 2) | (hi << 30);
		unsigned long hi2  = hi >> 2;

		unsigned long res_lo = lo + lo2;
		unsigned long carry  = (res_lo < lo);

		unsigned long res_mid = mid + mid2 + carry;
		carry = (res_mid < mid) || (res_mid == mid && carry);

		unsigned long res_hi = hi + hi2 + carry;

		if (res_hi < hi || (res_hi == hi && hi2 > 0)) {
			res_lo  = (res_lo >> 1)  | (res_mid << 31);
			res_mid = (res_mid >> 1) | (res_hi << 31);
			res_hi  = (res_hi >> 1)  | 0x80000000UL;
			exp_raw += 1;
		}

		/* Renormalize: if hi's bit 31 is not set, shift left until it is.
		 * This happens when exp was < 0 (value < 1) and the ×10 did not
		 * bring the value back into [1, 2).  Without this shift the
		 * mantissa drifts right across iterations and the digit
		 * extraction returns 0 for what should be significant digits. */
		if (!(res_hi & 0x80000000UL)) {
			int n;
			if (res_hi) {
				n = __builtin_clz(res_hi);
				res_hi  = (res_hi << n)  | (res_mid >> (32 - n));
				res_mid = (res_mid << n) | (res_lo >> (32 - n));
				res_lo  = (res_lo << n);
				exp_raw -= n;
			} else if (res_mid) {
				n = __builtin_clz(res_mid);
				res_hi  = (res_mid << n) | (res_lo >> (32 - n));
				res_mid = (res_lo << n);
				res_lo  = 0;
				exp_raw -= 32 + n;
			} else if (res_lo) {
				n = __builtin_clz(res_lo);
				res_hi  = (res_lo << n);
				res_mid = 0;
				res_lo  = 0;
				exp_raw -= 64 + n;
			}
			/* res_hi, res_mid, res_lo are all zero: value is zero,
			 * leave as is (the caller handles zero via the zero check). */
		}

		x->l.exp    = (unsigned short) (exp_raw + 3);
		x->l.high   = res_hi;
		x->l.middle = res_mid;
		x->l.low    = res_lo;

	}
	return (int) z;
}

LDL pow10_ld(LDL result, int n)
{
	/* 10^(2^bit) - 96-bit mantissa, round-to-nearest-even. */
	static const LDL pow10_pos_table[] = {
		{ .l = { 16386, 0xA0000000UL, 0x00000000UL, 0x00000000UL } },
		{ .l = { 16389, 0xC8000000UL, 0x00000000UL, 0x00000000UL } },
		{ .l = { 16396, 0x9C400000UL, 0x00000000UL, 0x00000000UL } },
		{ .l = { 16409, 0xBEBC2000UL, 0x00000000UL, 0x00000000UL } },
		{ .l = { 16436, 0x8E1BC9BFUL, 0x04000000UL, 0x00000000UL } },
		{ .l = { 16489, 0x9DC5ADA8UL, 0x2B70B59DUL, 0xF0200000UL } },
		{ .l = { 16595, 0xC2781F49UL, 0xFFCFA6D5UL, 0x3CBF6B72UL } },
		{ .l = { 16808, 0x93BA47C9UL, 0x80E98CDFUL, 0xC66F336CUL } },
		{ .l = { 17233, 0xAA7EEBFBUL, 0x9DF9DE8DUL, 0xDDBB901CUL } },
		{ .l = { 18083, 0xE319A0AEUL, 0xA60E91C6UL, 0xCC655C55UL } },
		{ .l = { 19784, 0xC9767586UL, 0x81750C17UL, 0x650D3D29UL } },
		{ .l = { 23186, 0x9E8B3B5DUL, 0xC53D5DE4UL, 0xA74D28CEUL } },
		{ .l = { 29989, 0xC4605202UL, 0x8A20979AUL, 0xC94C1540UL } }
	};

	/* 1 / 10^(2^bit) - 96-bit mantissa, round-to-nearest-even. */
	static const LDL pow10_neg_table[] = {
		{ .l = { 16379, 0xCCCCCCCCUL, 0xCCCCCCCCUL, 0xCCCCCCCDUL } },
		{ .l = { 16376, 0xA3D70A3DUL, 0x70A3D70AUL, 0x3D70A3D7UL } },
		{ .l = { 16369, 0xD1B71758UL, 0xE219652BUL, 0xD3C36113UL } },
		{ .l = { 16356, 0xABCC7711UL, 0x8461CEFCUL, 0xFDC20D2BUL } },
		{ .l = { 16329, 0xE69594BEUL, 0xC44DE15BUL, 0x4C2EBE68UL } },
		{ .l = { 16276, 0xCFB11EADUL, 0x453994BAUL, 0x67DE18EEUL } },
		{ .l = { 16170, 0xA87FEA27UL, 0xA539E9A5UL, 0x3F2398D7UL } },
		{ .l = { 15957, 0xDDD0467CUL, 0x64BCE4A0UL, 0xAC7CB3F7UL } },
		{ .l = { 15532, 0xC0314325UL, 0x637A1939UL, 0xFA911156UL } },
		{ .l = { 14682, 0x9049EE32UL, 0xDB23D21CUL, 0x7132D333UL } },
		{ .l = { 12981, 0xA2A682A5UL, 0xDA57C0BDUL, 0x87A60158UL } },
		{ .l = {  9579, 0xCEAE534FUL, 0x34362DE4UL, 0x492512D5UL } },
		{ .l = {  2776, 0xA6DD04C8UL, 0xD2CE9FDEUL, 0x2DE38124UL } }
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
