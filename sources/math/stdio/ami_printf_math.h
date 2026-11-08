/* ==========================================================================
 *  ami_printf_math.h
 *
 *  Shared floating-point math core for the narrow and wide printf engines.
 *
 *  Provides:
 *    - bit-pattern views of binary64 and Motorola 80-bit extended
 *    - software-only FP classification (no FPU compares, no libm)
 *    - signless 96-bit mantissa arithmetic (multiply, multiply-by-10)
 *    - base-10 digit extractor for long division
 *    - decimal exponent seeding (pow10_ld) with precomputed tables
 *
 *  This header is private to the printf implementation.  It is included by
 *  ami_printf_math.c (which provides the out-of-line definitions of the
 *  medium/large routines) and by vfprintf.c / vfwprintf.c (which consume the
 *  inline classifiers and comparison helpers and call the out-of-line ones).
 *
 *  Two views of long double coexist here:
 *
 *    Native Motorola 80-bit extended, big-endian, packed into 96 bits:
 *      bytes 0-1  : sign | 15-bit biased exponent (bias 16383)
 *      bytes 2-3  : padding -- completely ignored by the FPU
 *      bytes 4-11 : 64-bit mantissa, integer bit explicit in b63
 *      (union _ld_bits, used by the classifiers)
 *
 *    Internal working format, sign stripped, 96-bit mantissa:
 *      l.exp    : signed 32-bit exponent, normally biased by 16383
 *      l.high   : mantissa bits 95..64, integer bit explicit in b31
 *      l.middle : mantissa bits 63..32
 *      l.low    : mantissa bits 31..0
 *      (LDL, used by all arithmetic and digit extraction)
 *
 *    The exponent is signed because normalizing native subnormals can
 *    produce values below zero.
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
#ifndef AMI_PRINTF_MATH_H
#define AMI_PRINTF_MATH_H

#include <limits.h>
#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================
 * SECTION 1: bit-pattern views onto floating point values
 * ==================================================================== */

/*
 * binary64 ("double"), big-endian memory image:
 *
 *   byte 0..3 : seeeeeee eee(more) ffffffff ffffffff
 *   byte 4..7 : remaining fraction bits
 */
union _d_bits {
	double d;
	struct {
		unsigned sign :1;
		unsigned exp :11; /* biased by 1023          */
		unsigned frac0 :20; /* fraction bits 51..32    */
		unsigned frac1 :32; /* fraction bits 31..0     */
	} b;
	unsigned u;
};

/*
 * Native m68k extended precision ("long double"), 12 bytes (2 + 2 + 8):
 *
 *   bytes 0-1  : s eeeeeeeeeeeeeee      sign:1, exp:15 (bias 16383)
 *   bytes 2-3  : <pad>                  ignored by the FPU
 *   bytes 4-11 : 64-bit mantissa, integer bit explicit in b63
 *
 * Special patterns:
 *   inf  : exp $7FFF, mant $8000000000000000
 *   NaN  : exp $7FFF, any other mantissa  (b62 = quiet flag)
 *   zero : exp 0, mant 0                  (both polarities possible)
 *
 * The internal working format (LDL, below) widens this to a 96-bit
 * mantissa by adding a low 32-bit word and moving the integer bit to b95.
 */
union _ld_bits {
	long double ld;
	struct {
		unsigned sign :1;
		unsigned exp :15;
		unsigned pad :16; /* never interpret me */
		unsigned long long mant :64;
	} b;
};

/*
 * Internal working format. Signless, with a signed 32-bit exponent
 * followed by a 96-bit mantissa in three 32-bit words:
 *
 *   l.exp    : exponent (bias 16383 for normal values)
 *   l.high   : mantissa bits 95..64, integer bit explicit in b31
 *   l.middle : mantissa bits 63..32
 *   l.low    : mantissa bits 31..0
 *
 * The `s` view exposes the mantissa as six 16-bit words, for the
 * multiply routines that operate on 16-bit limbs.
 */
typedef union _internal {
	struct {
		long exp;
		unsigned long high; /* bits 95..64 */
		unsigned long middle; /* bits 63..32 */
		unsigned long low; /* bits 31..0  */
	} l;
	struct {
		long exp;
		unsigned short word[6];
	} s;
	struct {
		long exp;
		unsigned long dword[3];
	} i;
} LDL;

/* ======================================================================
 * SECTION 2: software-only FP classification
 *
 * Pure integer bit tests: no FPU compare instructions, no libm calls.
 * Small enough to be inlined everywhere; no out-of-line copy is emitted
 * once GCC drops the unused body (it is never address-taken).
 * ==================================================================== */

/* ----------------------- double ----------------------------------- */
static inline int _d_signbit(union _d_bits u) {
	return u.b.sign;
}
static inline int _d_isnan(union _d_bits u) {
	return u.b.exp == 0x7FF && (u.b.frac0 | u.b.frac1) != 0;
}
static inline int _d_isinf(union _d_bits u) {
	return u.b.exp == 0x7FF && u.b.frac0 == 0 && u.b.frac1 == 0;
}
/*
 * True only for exact zero (exp==0 AND fraction==0).
 * Denormals (exp==0, fraction!=0) intentionally report FALSE --
 * unlike ld == 0.0 they are genuinely nonzero values.
 */
static inline int _d_iszero(union _d_bits u) {
	return u.b.exp == 0 && u.b.frac0 == 0 && u.b.frac1 == 0;
}

/* ----------------------- long double ------------------------------ */
#define _LD_EXP_ALLONES  0x7FFF
#define _LD_INTBIT       0x8000000000000000ULL

static inline int _ld_signbit(union _ld_bits u) {
	return u.b.sign;
}
/* With an explicit integer bit, infinity is the ONLY $7FFF/$8000...000
 * pattern; every other all-exponent value is a NaN.                     */
static inline int _ld_isnan(union _ld_bits u) {
	return u.b.exp == _LD_EXP_ALLONES && u.b.mant != _LD_INTBIT;
}
static inline int _ld_isinf(union _ld_bits u) {
	return u.b.exp == _LD_EXP_ALLONES && u.b.mant == _LD_INTBIT;
}
static inline int _ld_iszero(union _ld_bits u) {
	return u.b.exp == 0 && u.b.mant == 0;
}

/* ======================================================================
 * SECTION 3: comparison helpers
 *
 * Lexicographic comparison of an LDL against a hardcoded 96-bit constant:
 *   1. Compare biased exponent.
 *   2. If equal, compare mantissa bits 95..64.
 *   3. If equal, compare mantissa bits 63..32.
 *   4. If equal, compare mantissa bits 31..0.
 *
 * Assumptions:
 *   - LDL is signless and already normalized.
 *   - No NaN, INF, or subnormal handling is required.
 *
 * Returns:
 *   -1  if a < constant
 *    0  if a == constant
 *    1  if a > constant
 *
 * Small enough to be inlined at every call site (S2 loops, S4 rounding).
 * ==================================================================== */
static inline int __LDL_cmp_const(const LDL *a,
		unsigned short bexp,
		unsigned long bhigh,
		unsigned long bmid,
		unsigned long blow)
{
	if (a->l.exp != bexp)
		return (a->l.exp < bexp) ? -1 : 1;
	if (a->l.high != bhigh)
		return (a->l.high < bhigh) ? -1 : 1;
	if (a->l.middle != bmid)
		return (a->l.middle < bmid) ? -1 : 1;
	if (a->l.low != blow)
		return (a->l.low < blow) ? -1 : 1;
	return 0;
}

/* Compare against 10.0:
 *   biased exp 0x4002 (2^3), 96-bit mantissa 0xA0000000 00000000 00000000
 */
static inline int __LDL_cmp_10(const LDL *a) {
	return __LDL_cmp_const(a, 0x4002, 0xA0000000UL, 0x00000000UL, 0x00000000UL);
}

/* Compare against a slightly reduced threshold for 5.0.
 *
 * The threshold is 0x9FFFFFFF_FFFFFF00_00000000 instead of the
 * exact 0xA0000000_00000000_00000000. The small epsilon compensates
 * for truncation drift accumulated during multiplication and scaling.
 */
static inline int __LDL_cmp_5(const LDL *a) {
	return __LDL_cmp_const(a, 0x4001, 0x9fffffffUL, 0xffffff00UL, 0x00000000UL);
}
static inline int __LDL_cmp_5U(const LDL *a) {
	return __LDL_cmp_const(a, 0x4001, 0xA0000000UL, 0x00000000UL, 0x00000000UL);
}

/* Compare against 1.0:
 *   biased exp 0x3FFF (2^0), 96-bit mantissa 0x80000000 00000000 00000000
 */
static inline int __LDL_cmp_1(const LDL *a) {
	return __LDL_cmp_const(a, 0x3FFF, 0x80000000UL, 0x00000000UL, 0x00000000UL);
}

/* ======================================================================
 * SECTION 4: out-of-line numeric core (defined in ami_printf_math.c)
 *
 * These routines are kept out of line so the narrow and wide printf
 * engines share a single implementation instead of duplicating the
 * arithmetic code in their respective text sections.
 * ==================================================================== */

/* Unpack a binary64 into the local 96-bit LDL view.  Handles denormals by
 * normalizing into the explicit-integer-bit layout; restores the implicit
 * hidden bit for normals; converts the exponent bias 1023 -> 16383.
 * The low 32 bits of the 96-bit mantissa are always zero for a double. */
LDL __LDL_from_double(double d);

/* Unpack an m68k extended-precision long double.  Copies the explicit bits
 * straight through; normalizes subnormals so the integer bit lands in b95.
 * The native mantissa has only 64 significant bits, so the low 32 bits of
 * the 96-bit LDL are always zero after the shift. */
LDL __LDL_from_long_double(long double ld);

/*
 * Multiply two 96-bit mantissas, normalize the result to an
 * integer bit at position 95, and combine the exponents.
 *
 * The multiplication precision is target-dependent:
 *   - 68000: 80-bit result (5 x 16-bit limbs)
 *   - 68020 and later: 96-bit result (6 x 16-bit limbs)
 *
 * The 68000 can also use the 96-bit implementation by setting N to 6,
 * at the cost of 11 additional MULU.W instructions per multiplication.
 *
 * Inputs are assumed normal and signless; zero is handled. No subnormal
 * or overflow generation is performed.
 *
 * Integer arithmetic is used throughout; no FPU operations are needed.
 */
LDL __LDL_mul(LDL a, LDL b);

/* Fast multiply by 10 using the identity 10 = 8 + 2 = (1 + 1/4) * 8,
 * i.e. shifts and adds on the 96-bit mantissa, then bias the exponent by 3. */
void __LDL_mul_10(LDL *x);

/* In-place multiply by 10 plus extraction of the leading base-10 digit.
 * Returns the digit [0..9]; x is updated to the fractional remainder,
 * renormalized so the integer bit is at position 95 of the mantissa. */
int __LDL_mul_10_extract(LDL *x);

/* Compute 10^n in the 96-bit working format by decomposing |n| into bits
 * and multiplying by the corresponding precomputed 10^(2^bit) table entry.
 * Positive n uses the 10^(+2^bit) table; negative n the 10^(-2^bit) one. */
LDL pow10_ld(LDL result, int n);

/*
 * Treat the mantissa as a plain 96-bit unsigned integer and perform:
 *
 *     *x = *x * 10 + digit,  0 <= digit <= 9
 *
 * The exponent is not modified. The result is not normalized; the
 * caller can normalize it afterwards using __LDL_normalize().
 *
 * Returns 1 if the digit was added successfully, 0 if the operation
 * would overflow the 96-bit mantissa.
 */
int __LDL_mul_10_add(LDL *x, int digit);

/*
 * Treat the mantissa as a plain 96-bit unsigned integer and perform:
 *
 *     *x = *x * 16 + digit,  0 <= digit <= 15
 *
 * The exponent is not modified and the result is not normalized.
 * Multiplication by 16 is a pure left shift by four bits.
 *
 * Returns 1 if the digit was added successfully, 0 if the operation
 * would overflow the 96-bit mantissa.
 */
int __LDL_mul_16_add(LDL *x, int digit);

/* Normalize an LDL so the integer bit is at b95.
 *
 * If the mantissa is already normalized, this is a no-op.  If it is
 * not (e.g. after a sequence of __LDL_mul_10_add calls that left the
 * integer bit below b95), shift left until b95 is set and reduce the
 * exponent by the shift amount.  A zero mantissa is left as zero.
 */
void __LDL_normalize(LDL *x);

/* Convert to float. sticky != 0 if there are more fractional digits != 0 */
float __LDL_to_float(const LDL *lv, unsigned int sticky);

/* Convert to double. sticky != 0 if there are more fractional digits != 0 */
double __LDL_to_double(const LDL *lv, unsigned int sticky);

/* Convert to long double. sticky != 0 if there are more fractional digits != 0 */
long double __LDL_to_long_double(const LDL *lv, unsigned int sticky);

/* ======================================================================
 * SECTION 5: shared constants and tuning knobs
 * ==================================================================== */

/* Working type for all float formatting: the 96-bit internal layout.  This
 * is what the digit extractor and the multiply helpers operate on. */
typedef LDL work_float_t;

/* 0.1 = 1.6 * 2^-4, used to nudge a value from [10,100) down to [1,10)
 * during S2 fine-tuning. */
extern const LDL WORK_0_1;

/* Significant decimal digits per source format.  Digit extraction goes this
 * deep (plus a guard digit) so rounding at the requested precision reflects
 * the source value, not an early cutoff. */
#ifdef DBL_DECIMAL_DIG
# define D_DIGS  DBL_DECIMAL_DIG
#else
# define D_DIGS  17
#endif
#ifdef LDBL_DECIMAL_DIG
# define LD_DIGS LDBL_DECIMAL_DIG
#else
# define LD_DIGS 21
#endif

/* log10(2) in Q32, used to seed the decimal exponent from the binary one. */
#define LOG10_2_Q32  1292913986U

/* Compile-time sanity checks for the assumed layouts (GCC only). */
#if defined(__GNUC__) && !defined(__cplusplus)
typedef char __chk_d_sz[(sizeof(double) == 8) ? 1 : -1];
typedef char __chk_ld_sz[(sizeof(long double) >= 10) ? 1 : -1];
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AMI_PRINTF_MATH_H */
