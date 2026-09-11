/* ==========================================================================
 *  stdio vfprintf.c
 *
 *  Minimal printf engine for AmigaOS / m68k (big-endian).
 *
 *  Conversions : d i o p u x X c s n %
 *                plus a A e E f F g G when FULL_SPECIFIERS is defined,
 *                including long double (length modifier 'L').
 *
 *  Long double layout (Motorola 80-bit extended, packed in 96 bits):
 *      bytes 0-1  : sign | 15-bit biased exponent (bias 16383)
 *      bytes 2-3  : padding -- completely ignored by the FPU
 *      bytes 4-11 : 64-bit mantissa with EXPLICIT integer bit in b63
 * ========================================================================== */
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
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
		unsigned exp :11; /* biased by 1023                     */
		unsigned frac0 :20; /* fraction bits 51..32               */
		unsigned frac1 :32; /* fraction bits 31..0                */
	} b;
	unsigned u;
};
/*
 * m68k extended precision ("long double"):
 *
 *   bytes 0-1 : s eeeeeeeeeeeeeee      sign:1, exp:15 (bias 16383)
 *   bytes 2-3 : <pad>                  hardware never reads these bits
 *   bytes 4-11: 64-bit mantissa, integer bit explicitly stored in b63
 *
 * Special patterns:
 *   inf  : exp $7FFF, mant $8000000000000000
 *   NaN  : exp $7FFF, any other mantissa  (b62 = quiet flag)
 *   zero : exp 0, mant 0                  (both polarities possible)
 */
union _ld_bits {
	long double ld;
	struct {
		unsigned sign :1;
		unsigned exp :15;
		unsigned pad :16; /* never interpret me   */
		unsigned long long mant :64;
	} b;
};
/* Compile-time sanity checks for the assumed layouts (GCC).            */
#if defined(__GNUC__)
typedef char __chk_d_sz[(sizeof(double) == 8) ? 1 : -1];
typedef char __chk_ld_sz[(sizeof(long double) >= 10) ? 1 : -1];
#endif
#define _D_EXP_ALLONES   0x7FF
#define _LD_EXP_ALLONES  0x7FFF
#define _LD_INTBIT       0x8000000000000000ULL
/* ======================================================================
 * SECTION 2: software-only FP classification
 *
 * Pure integer bit tests: no FPU compare instructions, no libm calls.
 * Two flavours each: _xxx_(union) for hot paths that already hold a
 * bits-view, and <name>_<w>(value) convenience wrappers.
 * ==================================================================== */
/* ----------------------- double ----------------------------------- */
static inline int _d_signbit(union _d_bits u) {
	return u.b.sign;
}
static inline int _d_isnan(union _d_bits u) {
	return u.b.exp == _D_EXP_ALLONES && (u.b.frac0 | u.b.frac1) != 0;
}
static inline int _d_isinf(union _d_bits u) {
	return u.b.exp == _D_EXP_ALLONES && u.b.frac0 == 0 && u.b.frac1 == 0;
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
/* --------------- value-taking wrappers ----------------------------- */
static inline int signbit_d(double v) {
	union _d_bits u;
	u.d = v;
	return _d_signbit(u);
}
static inline int isnan_d(double v) {
	union _d_bits u;
	u.d = v;
	return _d_isnan(u);
}
static inline int isinf_d(double v) {
	union _d_bits u;
	u.d = v;
	return _d_isinf(u);
}
static inline int iszero_d(double v) {
	union _d_bits u;
	u.d = v;
	return _d_iszero(u);
}
static inline int signbit_ld(long double v) {
	union _ld_bits u;
	u.ld = v;
	return _ld_signbit(u);
}
static inline int isnan_ld(long double v) {
	union _ld_bits u;
	u.ld = v;
	return _ld_isnan(u);
}
static inline int isinf_ld(long double v) {
	union _ld_bits u;
	u.ld = v;
	return _ld_isinf(u);
}
static inline int iszero_ld(long double v) {
	union _ld_bits u;
	u.ld = v;
	return _ld_iszero(u);
}

/* Check if GCC version is 16 or higher.  */
#if defined(__GNUC__) && (__GNUC__ >= 16)
#define USE_LDOUBLE_MATH 1
#else
#define USE_LDOUBLE_MATH 0
#endif

#if USE_LDOUBLE_MATH
/* Full long double precision math */
typedef long double work_float_t;
#define WORK_0_1    (0.1L  - 0.0000000000L)
#define WORK_10     (10.0L - 0.0000000000L)
#define WORK_5      (5.0L  - 0.0000000001L)
#else
/* Fallback: double precision math for older GCC */
typedef double work_float_t;
#define WORK_0_1    0.1
#define WORK_10     10.0
#define WORK_5      5.0
#endif
/* ======================================================================
 * SECTION 3: plumbing, flags, constants
 * ==================================================================== */
#ifdef FULL_SPECIFIERS
extern unsigned char *__decimalpoint;
#endif
extern int __vfprintf_total_size(FILE *stream, const char *fmt, va_list args);
/* Output one character and handle stream errors. */
/* A string stream (the snprintf family) that is full keeps counting: C99 says
 * the return value is the length the output would have had, and callers size a
 * buffer with vsnprintf(NULL, 0, ...) on the strength of that. putc on a full
 * __SSTR stream returns EOF from __swbuf without touching the buffer, so only
 * the count goes on. EOF from a real stream is still an error. */
#define OUT(c)  do { \
    if (putc((c), stream) == EOF && !(stream->_flags & __SSTR)) { \
        __STDIO_UNLOCK(stream); \
        return -1; \
    } \
    outcount++; \
} while(0)
#define TOUT(c) *outp++ = (c)
#define EOUT(c) *oute++ = (c)
/* Worst-case byte requirements per numeric family -> shared scratch buf */
#define MINFLOATSIZE   (DBL_DIG + 5)
#define MININTSIZE     (sizeof(unsigned long long) * CHAR_BIT / 3 + 1)
#define MINPOINTSIZE   (sizeof(void *) * CHAR_BIT / 4 + 1)
#define REQUIREDBUFFER ((MININTSIZE > MINPOINTSIZE                                        \
                            ? (MININTSIZE > MINFLOATSIZE ? MININTSIZE : MINFLOATSIZE)    \
                            : (MINPOINTSIZE > MINFLOATSIZE ? MINPOINTSIZE : MINFLOATSIZE))+8)
/* Flag bits; index in flagc[] == bit number.                           */
#define ALTERNATEFLAG  1        /* '#'   */
#define ZEROPADFLAG    2        /* '0'   */
#define LALIGNFLAG     4        /* '-'   */
#define BLANKFLAG      8        /* ' '   */
#define SIGNFLAG       16       /* '+'   */
#define HAS_PRECI      32       /* '.' seen */
static const char flagc[] = { '#', '0', '-', ' ', '+' };
extern unsigned __ulldivus(unsigned long long *llp, unsigned short n);
/*
 * Compute 10^n accurately using a precomputed table
 * of powers of 10 at exponents that are powers of two.
 *
 * For GCC >= 16, returns long double for full precision.
 * For older GCC, returns double (long double math is broken on m68k).
 */
#if USE_LDOUBLE_MATH
static long double pow10_ld(int n)
{
    static const long double pow10_table[] = {
        1e1L,          /* 10^1   */
        1e2L,          /* 10^2   */
        1e4L,          /* 10^4   */
        1e8L,          /* 10^8   */
        1e16L,         /* 10^16  */
        1e32L,         /* 10^32  */
        1e64L,         /* 10^64  */
        1e128L,        /* 10^128 */
        1e256L,        /* 10^256 */
        1e512L,        /* 10^512 */
        1e1024L,       /* 10^1024 */
        1e2048L,       /* 10^2048 */
        1e4096L,       /* 10^4096 */
    };
    long double result = 1.0L;
    int exp = n < 0 ? -n : n;
    int bit = 0;
    if (n == 0)
        return 1.0L;
    while (exp > 0) {
        if (exp & 1) {
            if (n < 0)
                result /= pow10_table[bit];
            else
                result *= pow10_table[bit];
        }
        exp >>= 1;
        bit++;
    }
    return result;
}
#else
/* Fallback for older GCC: use double precision */
static double pow10_ld(int n) {
	static const double pow10_table[] = { 1e1, /* 10^1   */
	1e2, /* 10^2   */
	1e4, /* 10^4   */
	1e8, /* 10^8   */
	1e16, /* 10^16  */
	1e32, /* 10^32  */
	1e64, /* 10^64  */
	1e128, /* 10^128 */
	1e256, /* 10^256 */
	1e512, /* 10^512 */
	1e1024, /* 10^1024 */
	1e2048, /* 10^2048 */
	1e4096, /* 10^4096 */
	};
	double result = 1.0;
	int exp = n < 0 ? -n : n;
	int bit = 0;
	if (n == 0)
		return 1.0;
	while (exp > 0) {
		if (exp & 1) {
			if (n < 0)
				result /= pow10_table[bit];
			else
				result *= pow10_table[bit];
		}
		exp >>= 1;
		bit++;
	}
	return result;
}
#endif
/*
 * How many significant decimal digits each source format genuinely
 * carries.  Digit extraction goes this deep (plus one guard digit) so
 * that rounding at the requested precision is honest rather than limited
 * by an early cutoff.
 */
#ifdef DBL_DECIMAL_DIG
# define D_DIGS  DBL_DECIMAL_DIG        /* typically 17                    */
#else
# define D_DIGS  17
#endif
#ifdef LDBL_DECIMAL_DIG
# define LD_DIGS LDBL_DECIMAL_DIG       /* 21 for 64-bit significand       */
#else
# define LD_DIGS 21                     /* m68k extended fallback          */
#endif
/* log10(2) as a 64-bit ratio, used to seed the decimal exponent
 * estimate directly from the binary exponent field.                    */
#define LOG2_10_NUM  3010299956639811952ULL
#define LOG2_10_DEN  10000000000000000000ULL
/* ======================================================================
 *  __vfprintf_total_size -- the formatter core.
 *  Returns the number of characters written, or -1 on stream error.
 * ==================================================================== */
/**
 *  Differs from vfprintf such that it returns total number of bytes that
 *  would've been written if there were sufficient space in file.
 *  Required for vsnprintf
 */
int __vfprintf_total_size(FILE *stream, const char *format, va_list args) {
	size_t outcount = 0;
	__STDIO_LOCK(stream);
	while (*format) {
		/* ============================================================
		 * Ordinary characters: copy through unchanged.
		 * ========================================================== */
		if (*format != '%') {
			OUT(*format++);
			continue;
		}
		/* ============================================================
		 * A '%' conversion begins here.
		 * ========================================================== */
		{
			static const char lowertabel[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
			static const char uppertabel[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
			short width = 0;
			unsigned short preci = 0x7fff; /* Sentinel value: no precision was specified. */
			short flags = 0;
			char type;
			char subtype = 'i';
			char buffer1[4]; /* sign / radix prefix slot */
			char buffer[REQUIREDBUFFER + 8];
			char outbuffer[REQUIREDBUFFER + 128]; /* Buffer used for output and left padding. */
			char *outp = outbuffer;
			char outexponent[8]; /* Separate buffer for the exponent, such as e+1234. */
			char *oute = 0; /* Set only when an exponent is emitted. */
			char *buffer2 = buffer;
			size_t size1 = 0, size2 = 0;
			const char *ptr = format + 1;
			unsigned short i, pad;
			/* ---- 1. flags -------------------------------------------
			 * Scan flagc[] repeatedly until a full pass finds no match
			 * (loop condition becomes false when i reaches the size).
			 * --------------------------------------------------------- */
			do {
				for (i = 0; i < sizeof(flagc); i++)
					if (flagc[i] == *ptr) {
						flags |= 1 << i;
						ptr++;
						break;
					}
			} while (i < sizeof(flagc));
			/* ---- 2. field width: literal digits or '*' --------------- */
			if (*ptr == '*') {
				signed int a;
				ptr++;
				a = va_arg(args, signed int);
				if (a < 0) { /* negative => left align   */
					flags |= LALIGNFLAG;
					width = -a;
				} else
					width = a;
			} else {
				while (isdigit(*ptr))
					width = width * 10 + (*ptr++ - '0');
			}
			/* ---- 3. precision: '.' digits or '.*' -------------------- */
			if (*ptr == '.') {
				flags |= HAS_PRECI;
				ptr++;
				if (*ptr == '*') /* read precision from arguments */
				{
					signed int a;
					ptr++;
					a = va_arg(args, signed int);
					if (a >= 0) /* negative means "unset"   */
						preci = a;
				} else {
					preci = 0;
					while (isdigit(*ptr))
						preci = preci * 10 + (*ptr++ - '0');
				}
			}
			/* ---- 4. length modifier ----------------------------------
			 * Internal codes: 'h' = halfword, 'i' = hh (byte),
			 * 'l' = long, 'm' = ll / intmax, 0 = none.
			 * --------------------------------------------------------- */
			if (*ptr == 'h' || *ptr == 'l' || *ptr == 'L' || *ptr == 'j' || *ptr == 'z' || *ptr == 't') {
				subtype = *ptr++;
				if (*ptr == 'h' || *ptr == 'l')
					++ptr, ++subtype;
			} else
				subtype = 0;
			type = *ptr++;
			switch (type) {
			/* ========================================================
			 * Integer family: d i o p u x X
			 *======================================================== */
			case 'd':
			case 'i':
			case 'o':
			case 'p':
			case 'u':
			case 'x':
			case 'X': {
				unsigned long long v;
				const char *tabel;
				int base;
				/* %p is spelled "%#lx" */
				if (type == 'p') {
					subtype = 'l'; /* This is written as %#lx */
					type = 'x';
					flags |= ALTERNATEFLAG;
				}
				if (type == 'd' || type == 'i') {
					/* ---- signed: magnitude -> v, prefix -> buffer1 ---- */
					signed long long v2;
					/* Extract the argument with proper type */
					if (subtype == 'l')
						v2 = va_arg(args, signed long);
					else if (subtype == 'm' || subtype == 'j')
						v2 = va_arg(args, signed long long);
					else
						v2 = va_arg(args, signed int);
					/* Convert hh/h arguments to their specified width. */
					if (subtype == 'h')
						v2 = (short) v2; /* Convert to signed short instead of masking the bits. */
					else if (subtype == 'i')
						v2 = (signed char) v2; /* Convert to signed char instead of masking the bits. */
					/* Now v2 is correctly sign-extended */
					if (v2 < 0) {
						if (size1 < sizeof(buffer1))
							buffer1[size1++] = '-';
						/* -LLONG_MIN cannot be negated in signed math    */
						v = (v2 == LLONG_MIN) ? (unsigned long long) LLONG_MAX + 1ULL : (unsigned long long) (-v2);
					} else {
						if (flags & SIGNFLAG)
							buffer1[size1++] = '+';
						else if (flags & BLANKFLAG)
							buffer1[size1++] = ' ';
						v = (unsigned long long) v2;
					}
				} else {
					/* ---- unsigned family ---- */
					if (subtype == 'l')
						v = va_arg(args, unsigned long);
					else if (subtype == 'm' || subtype == 'j')
						v = va_arg(args, unsigned long long);
					else
						v = va_arg(args, unsigned int);
					/*
					 * For unsigned types, bitmasks are correct because we want
					 * to truncate to the specified width (0-255 for char, 0-65535 for short)
					 */
					if (subtype == 'h')
						v &= 0xffffULL;
					else if (subtype == 'i')
						v &= 0xffULL;
					if (flags & ALTERNATEFLAG) {
						if (type == 'o') {
							/*
							 * For octal with # flag:
					         * - If value is 0: just output "0" (no extra prefix)
					         * - If precision is specified and value is 0: don't add prefix
					         * - Otherwise add '0' prefix
							 */
					        if (preci == 0x7fff && v != 0) {
								if (size1 < sizeof(buffer1))
									buffer1[size1++] = '0';
					        } else if (preci != 0x7fff && v != 0) {
					            /* Precision specified and value non-zero: add prefix if needed */
					            if (size1 < sizeof(buffer1)) {
					                buffer1[size1++] = '0';
					                --preci;
							}
					        }
					        /* If v == 0, don't add prefix - just let the digit loop output "0" */
						} else if ((type == 'x' || type == 'X') && v) {
							if (size1 + 2 <= sizeof(buffer1)) {
								buffer1[size1++] = '0';
					            buffer1[size1++] = type;
					            if (preci != 0x7fff)
					              preci -= 2;
							}
						}
					}
				}
				/* Divide down; digits land right-to-left in buffer.     */
				buffer2 = &buffer[sizeof(buffer)];
				base = (type == 'x' || type == 'X') ? 16 : (type == 'o') ? 8 : 10;
				tabel = (type != 'X') ? lowertabel : uppertabel;
				do {
					if (buffer2 <= buffer) /* Prevent writing before the start of the buffer. */
						break;
					*--buffer2 = tabel[__ulldivus(&v, base)];
					size2++;
				} while (v);
				/* Explicit precision = minimum digit count and it turns
				 * off '0' padding.                                     */
				if (preci == 0x7fff)
					preci = 0;
				else
					flags &= ~ZEROPADFLAG;
				break;
			}
				/* ========================================================
				 * %c
				 *======================================================== */
			case 'c': {
				char ch;
				if (subtype == 'l')
					ch = (char) va_arg(args, long);
				else
					ch = (char) va_arg(args, int);
				buffer[0] = ch;
				buffer2 = buffer;
				size2 = 1;
				preci = 0;
				break;
			}
				/* ========================================================
				 * %s  (NULL-safe)
				 *======================================================== */
			case 's': {
				buffer2 = va_arg(args, char*);
				if (buffer2 == NULL)
					buffer2 = "(null)";
				size2 = strlen(buffer2);
				if (size2 > preci)
					size2 = preci;
				preci = 0;
				break;
			}
#ifdef FULL_SPECIFIERS
				/* ========================================================
				 * Float family: a A e E f F g G
				 *
				 * Pipeline:
				 *   S1  fetch, strip sign into a char, classify specials
				 *       straight from the bit fields (no FPU compares)
				 *   S2  seed decimal exponent from the exponent field,
				 *       refine to [1,10) by scaling
				 *   S3  g/G style arbitration (fixed vs scientific)
				 *   S4  digit extraction + round-half-up
				 *   S5  trailing-zero trimming, width bookkeeping
				 *   S6  padded emission
				 *======================================================== */
			case 'a':
			case 'A':
			case 'f':
			case 'e':
			case 'E':
			case 'g':
			case 'G': {
				union _d_bits d;
				union _ld_bits lu; /* full-width ld operand */
				work_float_t ld_val; /* working precision     */
				int is_ld = (subtype == 'L');
				short exponent = 0; /* power of ten          */
				char sign = 0;
				const char *infnan = 0;
				char pad = ((flags & ZEROPADFLAG) && !(flags & LALIGNFLAG)) ? '0' : ' ';
#if USE_LDOUBLE_MATH
                const int MEANINGFUL_DIGITS = is_ld ? LD_DIGS : D_DIGS;
#else
				const int MEANINGFUL_DIGITS = D_DIGS;
#endif
				/* -------- S1: load + make magnitude + specials ------- */
				if (is_ld) {
					long double raw_ld = va_arg(args, long double);
					lu.ld = raw_ld;
#if USE_LDOUBLE_MATH
                    ld_val = raw_ld;             /* full precision        */
#else
					ld_val = (double) raw_ld; /* fall back to double   */
#endif
					if (_ld_signbit(lu)) {
						sign = '-';
						lu.b.sign = 0; /* erase sign field      */
#if USE_LDOUBLE_MATH
                        ld_val    = lu.ld;       /* reload operand        */
#else
						ld_val = (double) lu.ld;
#endif
					} else if (flags & SIGNFLAG)
						sign = '+';
					else if (flags & BLANKFLAG)
						sign = ' ';
					if (_ld_isnan(lu)) {
						infnan = "NaN";
						sign = 0; /* NaN carries no sign   */
					} else if (_ld_isinf(lu)) {
						infnan = "inf";
					}
				} else {
					d.d = va_arg(args, double);
					ld_val = d.d;
					if (_d_signbit(d)) {
						d.b.sign = 0;
						sign = '-';
						ld_val = d.d;
					} else if (flags & SIGNFLAG)
						sign = '+';
					else if (flags & BLANKFLAG)
						sign = ' ';
					if (_d_isnan(d)) {
						infnan = "NaN";
						sign = 0;
					} else if (_d_isinf(d)) {
						infnan = "inf";
					}
				}
				/* -------- S2: exponent extraction -------------------- */
				if (!infnan) {
					if (!is_ld ? !_d_iszero(d) : !_ld_iszero(lu)) {
						int bit_exp;
						int normalized_exp;
						if (is_ld) {
							bit_exp = lu.b.exp ? (int) lu.b.exp - 16383 : -16382;
						} else {
							bit_exp = d.b.exp ? d.b.exp - 1023 : -1022;
						}
						{
							unsigned long long e = (unsigned long long) (bit_exp >= 0 ? bit_exp : -bit_exp);
							normalized_exp = (int) (e * LOG2_10_NUM / LOG2_10_DEN);
							if (bit_exp < 0)
								normalized_exp = -normalized_exp;
						}
						/* Use pow10_ld() for accurate initial scaling */
						if (normalized_exp >= 0) {
							ld_val /= pow10_ld(normalized_exp);
						} else {
							ld_val *= pow10_ld(-normalized_exp);
						}
						/* Fine-tune to [1,10) with minimal iterations */
						while (ld_val >= WORK_10 && normalized_exp < 5000) {
							ld_val *= WORK_0_1;
							++normalized_exp;
						}
						while (ld_val < 1.0 && normalized_exp > -5000) {
							ld_val *= WORK_10;
							--normalized_exp;
						}
						exponent = (short) normalized_exp;
					} else {
						exponent = 0;
					}
				}
				/* ------------------------------------------------------
				 * S3-S5: digit production, rounding, trimming, width
				 * ---------------------------------------------------- */
				{
					int pos = 0;
					int startPos;
					short leading = 1;
					short killZero = 0;
					int digits_to_compute = MEANINGFUL_DIGITS;
					short isg = 0;
					if (preci == 0x7fff) {
						if (type == 'f' || type == 'F' || type == 'e' || type == 'E' || type == 'g' || type == 'G') {
							/* ISO C standard requires a default precision of 6 for floating-point formats */
							preci = 6;
						} else if (type == 'a' || type == 'A') {
							/* -1 signals the hex formatter to output enough digits for an exact value */
							preci = -1;
						} else {
							preci = 1;
						}
					}
					if (!infnan) {
						if (type != 'a' && type != 'A') {
							if (type == 'g' || type == 'G') {
								int original_preci = (preci == 0) ? 1 : preci;
								isg = 1;
								if (exponent >= -4 && exponent < original_preci) {
									--type; /* Fixed-point 'f' or 'F' */
									/* Calculate digits_to_compute for %f. */
									if (flags & ALTERNATEFLAG) {
										/* With #, preci specifies the number of digits after the decimal point. */
										preci = original_preci;
									}
									if (exponent >= 0) {
										if ((flags & ALTERNATEFLAG) && preci == exponent + 1) {
											++preci;
										}
										digits_to_compute = preci; /* Use preci directly. */
									} else {
										/* For a negative exponent, the value is less than 1. */
										if (flags & ALTERNATEFLAG) {
											/* With #, preci specifies the number of digits after the decimal point. */
											digits_to_compute = 1 + preci + 1; /* One leading zero, preci fractional digits, and one guard digit. */
										} else {
											digits_to_compute = preci;
										}
									}
								} else {
									type -= 2; /* 'e' or 'E'. */
									digits_to_compute = preci;
								}
								if (!(flags & ALTERNATEFLAG)) {
									killZero = 1;
								}
							} else if (type == 'f' || type == 'F') {
								if (exponent >= 0) {
									digits_to_compute = exponent + preci + 1;
								} else {
									digits_to_compute = preci + exponent;
									if (digits_to_compute < 0)
										digits_to_compute = 0;
									++digits_to_compute;
								}
							} else if (type == 'e' || type == 'E') {
								digits_to_compute = preci + 1;
								if (digits_to_compute > MEANINGFUL_DIGITS)
									digits_to_compute = MEANINGFUL_DIGITS;
							} else {
								digits_to_compute = preci + 2;
							}
							if (digits_to_compute <= 0)
								digits_to_compute = 1;
							else if (digits_to_compute > MEANINGFUL_DIGITS) {
								digits_to_compute = MEANINGFUL_DIGITS;
							}
							/* Use ld_val for digit extraction, not d.d */
							work_float_t work = ld_val;
							buffer[0] = '0';
							startPos = pos = 1;
							/* Compute digits using ld_val */
							for (; pos <= digits_to_compute && pos < sizeof(buffer); ++pos) {
								int z = (int) work;
								if (z < 0) z = 0;
								else if (z > 9) z = 9;
								buffer[pos] = '0' + z;
								work = (work - z) * WORK_10;
							}
							/* Perform rounding */
							if (work >= WORK_5 - 0.0000001) {
								int tpos = pos - 1;
								while (tpos > 0) {
									if (++buffer[tpos] <= '9')
										break;
									buffer[tpos--] = '0';
								}
								/* Handle carry overflow (e.g., 9.99 -> 10.00 or 0.099 -> 0.100) */
								if (tpos == 0) {
									startPos = 0;
									buffer[0] = '1';
									exponent++; /* Exponent shifts up globally for all format paths */
									if (exponent > 0) {
										++leading;
										++ digits_to_compute;
									}
								} else if (exponent + preci < 0 && buffer[tpos] == '5') {
									startPos = 0;
									buffer[0] = '1';
									exponent++; /* Exponent shifts up globally for all format paths */
								}
							}
							/* Set the number of integer digits; this matters only for %f. */
							if (type == 'f' || type == 'F') {
								leading = (exponent >= 0) ? (exponent + 1) : 0;
							} else {
								leading = 1;
							}
						} else {
							/* 'a' 'A' format - pure bit operations */
							if (!(flags & HAS_PRECI))
								killZero = 1;
							TOUT('0');
							TOUT(type + ('X' - 'A'));
							width -= 2;
							buffer[0] = '0';
							startPos = 0;
							pos = 1;
							if (is_ld) {
								if (lu.b.exp == 0) {
									if (lu.b.mant == 0) {
										exponent = 0;
									} else {
										int shift = __builtin_clzll(lu.b.mant);
										exponent = -16382 - (64 - shift);
									}
								} else {
									exponent = (short) (lu.b.exp - 16383);
									buffer[0] = '1';
								}
								/* Extract mantissa for long double */
								unsigned x = (unsigned) (lu.b.mant >> 32);
								unsigned y = (unsigned) (lu.b.mant & 0xFFFFFFFFULL);
								/* Full 64-bit mantissa including explicit integer bit */
								unsigned long long mant = lu.b.mant;
								/* Drop the explicit integer bit (bit 63) */
								mant &= 0x7FFFFFFFFFFFFFFFULL;
								/* Extract 16 hex digits (64 bits) */
								for (int i = 0; i < 16; i++) {
								    int digit = (mant >> (60 - 4*i)) & 0xF;
								    buffer[pos++] = "0123456789abcdef"[digit];
								}
							} else {
								if (!d.b.exp) {
									exponent = 0;
								} else {
									exponent = d.b.exp - 1023;
									buffer[0] = '1';
								}
								/* Extract 52-bit mantissa for double */
								unsigned long long mant = ((unsigned long long)d.b.frac0 << 32) | d.b.frac1;
								/* Drop implicit integer bit */
								mant &= 0xFFFFFFFFFFFFFULL;
								/* Print 13 hex digits (52 bits) */
								for (int i = 0; i < 13; i++) {
								    int digit = (mant >> (48 - 4*i)) & 0xF;
								    buffer[pos++] = "0123456789abcdef"[digit];
								}
							}
							type += 'P' - 'A';
							leading = 1;
						}
					}
					if (preci > 0x3fff)
						preci = 1;
					/* -------- S6: Output generation -------- */
					if (sign != 0) {
						TOUT(sign);
						--width;
					}
					if (infnan) {
						TOUT(infnan[0]);
						TOUT(infnan[1]);
						TOUT(infnan[2]);
						width -= 3;
						pad = ' ';
					} else {
						int fractional_output = 0;
						/* 1. Print digits before the decimal point */
						if (exponent < 0 && (type == 'f' || type == 'F')) {
							/* For small fixed-point numbers (e.g., 0.00123), print the mandatory leading '0' */
							TOUT('0');
							--width;
						} else {
							if (isg)
								preci -= leading;
							int rpad = exponent - leading;
							while (startPos < pos && leading-- > 0) {
								TOUT(buffer[startPos++]);
								--width;
								if (isg && !(flags & ALTERNATEFLAG)) {
									--preci;
									if (preci > 0x3fff)
										preci = 0;
								}
							}
							while (leading-- > 0)
								TOUT('0');
							if (type == 'f' || type == 'F') {
								while (rpad-- > 0)
									TOUT('0');
							}
						}
						if (killZero) {
							/* Remove trailing zeros from the complete digit buffer. */
							int stop = pos - 1;
							while (stop >= startPos && buffer[stop] == '0') {
								stop--;
							}
							preci = stop - startPos + 1;   /* Number of significant digits. */
							if (preci > 0x3fff)
								preci = 0;
							pos = stop + 1;
						}
						/* 2. Print the decimal point if required */
						if (preci || (flags & ALTERNATEFLAG) != 0) {
							TOUT((__decimalpoint && __decimalpoint[0]) ? __decimalpoint[0] : '.');
							--width;
						}
						/* limit output */
						if (pos > startPos + preci) {
							pos = startPos + preci;
						}
						/* 3. Print leading fractional zeros for small %f numbers */
						if (exponent < 0 && (type == 'f' || type == 'F')) {
							int fractional_zeros = -exponent - 1; /* e.g., exp = -3 -> 2 zeros */
							while (fractional_zeros-- > 0 && preci > 0) {
								TOUT('0');
								--width;
								if (!isg)
									--preci;
								fractional_output++;
							}
						}
						/* 4. Print the remaining significant digits from the buffer */
						while (preci > 0 && startPos < pos) {
							TOUT(buffer[startPos++]);
							--width;
							--preci;
						}
						/* 5. Pad trailing zeros to satisfy the requested precision */
						int zeros_to_add = 0;
						if (!(type == 'f' || type == 'F')) {
							/* For %e, %g (scientific), and %a: precision dictates exact trailing zeros */
							zeros_to_add = preci - fractional_output;
						} else {
							/* For %f and %g (fixed-point): track against global preci constraint */
							zeros_to_add = preci - fractional_output;
						}
						if (zeros_to_add < 0)
							zeros_to_add = 0;
						while (zeros_to_add-- > 0) {
							TOUT('0');
							--width;
						}
						if (!(type == 'f' || type == 'F')) {
							int xout = 0;
							char t_lower = (char) (type | 0x20); /* Convert to lowercase ('e' or 'p') */
							oute = outexponent;
							EOUT(type);
							if (exponent < 0) {
								EOUT('-');
								exponent = -exponent;
							} else {
								EOUT('+');
							}
							/* Thousands digit (e.g., for long double extremes like e+4932) */
							if (exponent > 999) {
								int z = exponent / 1000;
								EOUT('0' + z);
								exponent -= z * 1000;
								xout = 1;
							}
							/* Hundreds digit */
							if (xout || exponent > 99) {
								int z = exponent / 100;
								EOUT('0' + z);
								exponent -= z * 100;
								xout = 1;
							}
							/* Tens digit: Forced to print for 'e'/'E' to guarantee at least 2 digits (e+00) */
							if (xout || exponent > 9 || t_lower == 'e') {
								int z = exponent / 10;
								EOUT('0' + z);
								exponent -= z * 10;
							}
							/* Units digit (always printed) */
							EOUT('0' + exponent);
						}
					}
					if (oute)
						width -= oute - outexponent;
					if ((flags & LALIGNFLAG) != 0) {
						/* LEFT ALIGNED: print temp then pad */
						for (char *p = outbuffer; p < outp; ++p)
							OUT(*p);
						if (oute) {
							for (char *p = outexponent; p < oute; ++p)
								OUT(*p);
						}
						while (--width >= 0)
							OUT(' ');
					} else {
						/* RIGHT ALIGNED: pad then print */
						char *qout = outbuffer;
						/* check for sign */
						if (pad == '0' && (*qout == '-' || *qout == '+' || *qout == ' ')) {
							OUT(*qout++); /* Print the sign before zero padding. */
							while (--width >= 0)
								OUT('0');
						} else {
							while (--width >= 0)
								OUT(pad);
						}
						for (; qout < outp; ++qout)
							OUT(*qout);
						if (oute) {
							for (char *p = outexponent; p < oute; ++p)
								OUT(*p);
						}
					}
					format = ptr;
					continue;
				}
			}
#endif
			case '%':
				buffer2 = "%";
				size2 = 1;
				preci = 0;
				break;
			case 'n':
				*va_arg(args, int*) = outcount;
				width = preci = 0;
				break;
			default:
				if (!type)
					ptr--; /* We've gone too far - step one back */
				buffer2 = (char*) format;
				size2 = ptr - format;
				width = preci = 0;
				break;
			}
			if ((flags & HAS_PRECI) && !(flags & ALTERNATEFLAG)) {
				if (size2 == 1 && !preci && buffer2[0] == '0')
					size2 = 0;
			}
			pad = size1 + (size2 >= preci ? size2 : preci); /* Calculate the number of characters */
			pad = pad >= width ? 0 : width - pad; /* and the number of resulting pad bytes */
			if (flags & ZEROPADFLAG) /* Print the sign and prefix. */
				for (i = 0; i < size1; i++)
					OUT(buffer1[i]);
			if (!(flags & LALIGNFLAG)) /* Pad left */
				for (i = 0; i < pad; i++)
					OUT(flags&ZEROPADFLAG?'0':' ');
			if (!(flags & ZEROPADFLAG)) /* Print the sign when zero padding is not used. */
				for (i = 0; i < size1; i++)
					OUT(buffer1[i]);
			for (i = size2; i < preci; i++) /* extend to precision */
				OUT('0');
			for (i = 0; i < size2; i++) /* Print the converted value. */
				OUT(buffer2[i]);
			if (flags & LALIGNFLAG) /* Pad right */
				for (i = 0; i < pad; i++)
					OUT(' ');
			format = ptr;
		}
	}
	__STDIO_UNLOCK(stream);
	return outcount;
}
/* ======================================================================
 * Self-test harness (build with -DTESTME).
 * ==================================================================== */
#ifdef TESTME
#include <string.h>
/* Test counter */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;
/*
 * Classifier display functions - poke union fields directly to verify
 * the bit-level classification logic.
 */
static void show_d(const char *tag, double v) {
	union _d_bits u;
	u.d = v;
	printf("  %-6s sign=%d nan=%d inf=%d zero=%d\n", tag, _d_signbit(u), _d_isnan(u), _d_isinf(u), _d_iszero(u));
}
static void show_ld(const char *tag, long double v) {
	union _ld_bits u;
	u.ld = v;
	printf("  %-6s sign=%d nan=%d inf=%d zero=%d\n", tag, _ld_signbit(u), _ld_isnan(u), _ld_isinf(u), _ld_iszero(u));
}
/*
 * Check that snprintf output matches an expected string exactly.
 *
 * The 'combined' string has format: "label|expected|"
 * The actual format string is passed separately in 'fmt'.
 * We find the LAST '|' to split expected from the format string portion,
 * then find the FIRST '|' in the prefix to split label from expected.
 */
static int check_fmt(const char *combined, const char *fmt, ...) {
	char buf[512];
	const char *expected;
	const char *label;
	const char *last_sep;
	const char *first_sep;
	size_t expected_len;
	int ret;
	va_list args;
	/* Find the last '|' which terminates the expected value */
	last_sep = strrchr(combined, '|');
	if (!last_sep) {
		/* No separator at all - nothing to compare */
		printf("BUG: no '|' in test spec: %s\n", combined);
		tests_run++;
		tests_failed++;
		return 0;
	}
	/* Find the first '|' which separates label from expected */
	first_sep = strchr(combined, '|');
	if (first_sep == last_sep) {
		/* Only one '|' - no label, just expected value */
		label = "(unnamed)";
		expected = combined;
		expected_len = (size_t) (last_sep - combined);
	} else {
		/* label|expected| */
		label = combined;
		expected = first_sep + 1;
		expected_len = (size_t) (last_sep - expected);
	}
	va_start(args, fmt);
	ret = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	tests_run++;
	if (ret == (int) expected_len && memcmp(buf, expected, expected_len) == 0) {
		printf("OK: %s\n", label);
		tests_passed++;
		return 1;
	} else {
		tests_failed++;
		printf("FAIL: %s\n", label);
		printf("      fmt:      \"%s\"\n", fmt);
		printf("      expected: \"");
		fwrite(expected, 1, expected_len, stdout);
		printf("\" (len=%zu)\n", expected_len);
		printf("      got:      \"");
		fwrite(buf, 1, (size_t) ret, stdout);
		printf("\" (len=%d)\n", ret);
		return 0;
	}
}
/*
 * Macro wrapper: TESTFMT("description|expected_result|", "format", args...)
 *
 * The trailing '|' after expected_result is required to mark the end.
 * The actual format string is passed as the second argument.
 */
#define TESTFMT(desc, fmt, ...) \
    check_fmt(desc, fmt, ##__VA_ARGS__)
int main(int argc, char **argv) {
	double d;
	(void) argc;
	(void) argv;
	printf("=== vfprintf test harness ===\n\n");
	/* Print which math mode we're using */
#if USE_LDOUBLE_MATH
	printf("Math mode: long double (GCC >= 16)\n\n");
#else
	printf("Math mode: double fallback (GCC < 16)\n\n");
#endif
	/* ==================================================================
	 * SECTION 1: Bit-level classifier tests
	 * ================================================================== */
	printf("--- Classifier tests ---\n");
	{
		union _d_bits td;
		union _ld_bits tl;
		printf("  Double classifiers:\n");
		td.d = 0.0;
		show_d("zero", td.d);
		td.b.exp = 0x7ff;
		show_d("+inf", td.d);
		td.b.frac1 = 1;
		show_d("nan", td.d);
		td.b.exp = 0;
		td.b.frac1 = 0;
		td.b.sign = 1;
		show_d("-0", td.d);
		td.b.sign = 0;
		td.b.frac1 = 1;
		show_d("den", td.d);
		printf("  Long double classifiers:\n");
		tl.ld = 0.0L;
		show_ld("zero", tl.ld);
		tl.b.exp = 0x7fff;
		tl.b.mant = _LD_INTBIT;
		show_ld("+inf", tl.ld);
		tl.b.mant = 0xC000000000000000ULL;
		show_ld("qNaN", tl.ld);
		tl.b.mant = 0x4000000000000000ULL;
		show_ld("sNaN", tl.ld);
		tl.b.exp = 0;
		tl.b.mant = 0;
		tl.b.sign = 1;
		show_ld("-0", tl.ld);
	}
	printf("\n");
	/* ==================================================================
	 * SECTION 2: Zero and alternate-form tests
	 * ================================================================== */
	printf("--- Zero / alternate-form tests ---\n");
	TESTFMT("zero fixed left pad|0                   |", "%-20.0f", 0.0);
	TESTFMT("zero sci left pad|0e+00               |", "%-20.0e", 0.0);
	TESTFMT("zero g left pad|0                   |", "%-20.1g", 0.0);
	TESTFMT("zero fixed alt right pad|                  0.|", "%#20.0f", 0.0);
	TESTFMT("zero sci alt right pad|              0.e+00|", "%#20.0e", 0.0);
	TESTFMT("zero g alt right pad|                 0.0|", "%#20.1g", 0.0);
	TESTFMT("negative zero|-0|", "%.0f", -0.0);
	TESTFMT("zero with plus|+0|", "%+.0f", 0.0);
	TESTFMT("zero with space| 0|", "% .0f", 0.0);
	TESTFMT("zero prec 5|0.00000|", "%.5f", 0.0);
	TESTFMT("simple 5.0|5|", "%.0f", 5.0);
	printf("\n");
	/* ==================================================================
	 * SECTION 3: Fixed-point (%f) decade walk
	 * ================================================================== */
	printf("--- Fixed-point %%f decade walk ---\n");
	d = 1.2345678902468e-13;
	TESTFMT("f e-13|           0.0000000|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e-12|           0.0000000|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e-11|           0.0000000|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e-10|           0.0000000|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e-9 |           0.0000000|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e-8 |           0.0000000|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e-7 |           0.0000001|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e-6 |           0.0000012|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e-5 |           0.0000123|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e-4 |           0.0001235|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e-3 |           0.0012346|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e-2 |           0.0123457|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e-1 |           0.1234568|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e0  |           1.2345679|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e1  |          12.3456789|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e2  |         123.4567890|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e3  |        1234.5678902|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e4  |       12345.6789025|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e5  |      123456.7890247|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e6  |     1234567.8902468|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e7  |    12345678.9024680|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e8  |   123456789.0246800|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e9  |  1234567890.2468000|", "%20.7f", d);
	d *= 10;
	TESTFMT("f e10 | 12345678902.4680000|", "%20.7f", d);
	printf("\n");
	/* ==================================================================
	 * SECTION 4: General format (%g) decade walk
	 * ================================================================== */
	printf("--- General %%g decade walk ---\n");
	d = 1.2345678902468e-13;
	TESTFMT("g e-13|        1.234568e-13|", "%20.7g", d); // value length = 12 -> 8 spaces
	d *= 10;
	TESTFMT("g e-12|        1.234568e-12|", "%20.7g", d); // value length = 12 -> 8 spaces
	d *= 10;
	TESTFMT("g e-11|        1.234568e-11|", "%20.7g", d); // value length = 12 -> 8 spaces
	d *= 10;
	TESTFMT("g e-10|        1.234568e-10|", "%20.7g", d); // value length = 12 -> 8 spaces
	d *= 10;
	TESTFMT("g e-9 |        1.234568e-09|", "%20.7g", d); // value length = 12 -> 8 spaces
	d *= 10;
	TESTFMT("g e-8 |        1.234568e-08|", "%20.7g", d); // value length = 12 -> 8 spaces
	d *= 10;
	TESTFMT("g e-7 |        1.234568e-07|", "%20.7g", d); // value length = 12 -> 8 spaces
	d *= 10;
	TESTFMT("g e-6 |        1.234568e-06|", "%20.7g", d); // value length = 12 -> 8 spaces
	d *= 10;
	TESTFMT("g e-5 |        1.234568e-05|", "%20.7g", d); // (value length = 12 -> 8 spaces)
	d *= 10;
	TESTFMT("g e-4 |        0.0001234568|", "%20.7g", d); // (value length = 12 -> 8 spaces instead of 5)
	d *= 10;
	TESTFMT("g e-3 |         0.001234568|", "%20.7g", d); // (value length = 11 -> 9 spaces instead of 6)
	d *= 10;
	TESTFMT("g e-2 |          0.01234568|", "%20.7g", d); // (value length = 10 -> 10 spaces instead of 7)
	d *= 10;
	TESTFMT("g e-1 |           0.1234568|", "%20.7g", d); // (value length = 9 -> 11 spaces instead of 7)
	d *= 10;
	TESTFMT("g e0  |            1.234568|", "%20.7g", d); // (value length = 8 -> 12 spaces instead of 7)
	d *= 10;
	TESTFMT("g e1  |            12.34568|", "%20.7g", d); // (value length = 8 -> 12 spaces instead of 7)
	d *= 10;
	TESTFMT("g e2  |            123.4568|", "%20.7g", d); // (value length = 8 -> 12 spaces instead of 7)
	d *= 10;
	TESTFMT("g e3  |            1234.568|", "%20.7g", d); // (value length = 8 -> 12 spaces instead of 7)
	d *= 10;
	TESTFMT("g e4  |            12345.68|", "%20.7g", d); // (value length = 8 -> 12 spaces instead of 7)
	d *= 10;
	TESTFMT("g e5  |            123456.8|", "%20.7g", d); // (value length = 8 -> 12 spaces instead of 7)
	d *= 10;
	TESTFMT("g e6  |             1234568|", "%20.7g", d); // (value length = 7 -> 13 spaces instead of 7)
	d *= 10;
	TESTFMT("g e7  |        1.234568e+07|", "%20.7g", d); // value length = 12 -> 8 spaces
	d *= 10;
	TESTFMT("g e8  |        1.234568e+08|", "%20.7g", d); // value length = 12 -> 8 spaces
	d *= 10;
	TESTFMT("g e9  |        1.234568e+09|", "%20.7g", d); // value length = 12 -> 8 spaces
	d *= 10;
	TESTFMT("g e10 |        1.234568e+10|", "%20.7g", d); // value length = 12 -> 8 spaces
	printf("\n");
	/* ==================================================================
	 * SECTION 5: Scientific (%e) decade walk
	 * ================================================================== */
	printf("--- Scientific %%e decade walk ---\n");
	d = 1.2345678902468e-13;
	TESTFMT("e e-13|       1.2345679e-13|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e-12|       1.2345679e-12|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e-11|       1.2345679e-11|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e-10|       1.2345679e-10|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e-9 |       1.2345679e-09|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e-8 |       1.2345679e-08|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e-7 |       1.2345679e-07|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e-6 |       1.2345679e-06|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e-5 |       1.2345679e-05|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e-4 |       1.2345679e-04|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e-3 |       1.2345679e-03|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e-2 |       1.2345679e-02|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e-1 |       1.2345679e-01|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e0  |       1.2345679e+00|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e1  |       1.2345679e+01|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e2  |       1.2345679e+02|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e3  |       1.2345679e+03|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e4  |       1.2345679e+04|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e5  |       1.2345679e+05|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e6  |       1.2345679e+06|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e7  |       1.2345679e+07|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e8  |       1.2345679e+08|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e9  |       1.2345679e+09|", "%20.7e", d);
	d *= 10;
	TESTFMT("e e10 |       1.2345679e+10|", "%20.7e", d);
	printf("\n");
	/* ==================================================================
	 * SECTION 6: Rounding tests
	 * ================================================================== */
	printf("--- Rounding tests ---\n");
	TESTFMT("round up .5|2|", "%.0f", 1.5);
	TESTFMT("round up .6|2|", "%.0f", 1.6);
	TESTFMT("round down .4|1|", "%.0f", 1.4);
	TESTFMT("round 9 cascade|10|", "%.0f", 9.5);
	TESTFMT("round 99 cascade|100|", "%.0f", 99.5);
	TESTFMT("round .05|1.1|", "%.1f", 1.05);
	TESTFMT("round .15|1.2|", "%.1f", 1.15);
	TESTFMT("round .25|1.3|", "%.1f", 1.25);
	TESTFMT("round .35|1.4|", "%.1f", 1.35);
	TESTFMT("round .45|1.5|", "%.1f", 1.45);
	TESTFMT("round .55|1.6|", "%.1f", 1.55);
	TESTFMT("round .65|1.7|", "%.1f", 1.65);
	TESTFMT("round .75|1.8|", "%.1f", 1.75);
	TESTFMT("round .85|1.9|", "%.1f", 1.85);
	TESTFMT("round .95|2.0|", "%.1f", 1.95);
	printf("\n");
	/* ==================================================================
	 * SECTION 7: Special values (inf, NaN)
	 * ================================================================== */
	printf("--- Special value tests ---\n");
	{
		union _d_bits inf_bits, nan_bits;
		double pos_inf, neg_inf, qnan;
		inf_bits.d = 0.0;
		inf_bits.b.exp = 0x7ff;
		pos_inf = inf_bits.d;
		inf_bits.b.sign = 1;
		neg_inf = inf_bits.d;
		nan_bits.d = 0.0;
		nan_bits.b.exp = 0x7ff;
		nan_bits.b.frac0 = 1;
		qnan = nan_bits.d;
		TESTFMT("+inf|inf|", "%f", pos_inf);
		TESTFMT("-inf|-inf|", "%f", neg_inf);
		TESTFMT("nan|NaN|", "%f", qnan);
		TESTFMT("+inf padded|       inf|", "%10f", pos_inf);
		TESTFMT("-inf padded|      -inf|", "%10f", neg_inf);
		TESTFMT("nan padded|       NaN|", "%10f", qnan);
		TESTFMT("+inf sci|inf|", "%e", pos_inf);
		TESTFMT("nan g|NaN|", "%g", qnan);
	}
	printf("\n");
	/* ==================================================================
	 * SECTION 8: Padding and alignment tests
	 * ================================================================== */
	printf("--- Padding / alignment tests ---\n");
	TESTFMT("right pad zero|0000042|", "%07d", 42);
	TESTFMT("left pad space|42     |", "%-7d", 42);
	TESTFMT("right pad space|     42|", "%7d", 42);
	TESTFMT("float zero pad|000012.35|", "%09.2f", 12.345);
	TESTFMT("float space pad|    12.35|", "%9.2f", 12.345);
	TESTFMT("float left pad|12.35    |", "%-9.2f", 12.345);
	TESTFMT("sign prefix|+12.35|", "%+.2f", 12.345);
	TESTFMT("space prefix| 12.35|", "% .2f", 12.345);
	TESTFMT("neg sign|-12.35|", "%.2f", -12.345);
	TESTFMT("neg zero pad|-00012.35|", "%09.2f", -12.345);
	printf("\n");
	/* ==================================================================
	 * SECTION 9: Integer format tests
	 * ================================================================== */
	printf("--- Integer format tests ---\n");
	TESTFMT("dec basic|42|", "%d", 42);
	TESTFMT("dec neg|-42|", "%d", -42);
	TESTFMT("dec zero|0|", "%d", 0);
	TESTFMT("oct basic|52|", "%o", 42);
	TESTFMT("oct alt|052|", "%#o", 42);
	TESTFMT("hex lower|2a|", "%x", 42);
	TESTFMT("hex upper|2A|", "%X", 42);
	TESTFMT("hex alt lower|0x2a|", "%#x", 42);
	TESTFMT("hex alt upper|0X2A|", "%#X", 42);
	TESTFMT("unsigned|42|", "%u", 42);
	TESTFMT("unsigned max|4294967295|", "%u", 0xFFFFFFFFU);
	TESTFMT("long dec|1234567890|", "%ld", 1234567890L);
	TESTFMT("char|A|", "%c", 'A');
	TESTFMT("string|hello|", "%s", "hello");
	TESTFMT("null string|(null)|", "%s", (char* )0);
	TESTFMT("percent|%|", "%%");
	TESTFMT("prec zero||", "%.0d", 0);
	TESTFMT("prec zero alt|0|", "%#.0d", 0);
	printf("\n");
	/* ==================================================================
	 * SECTION 10: Pointer format tests
	 * ================================================================== */
	printf("--- Pointer format tests ---\n");
	/* Note: NULL pointer format is implementation-defined, common is "(nil)" or "0x0" */
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "%p", (void*) 0);
		printf("  NULL pointer prints as: \"%s\" (implementation-defined)\n", buf);
	}
	printf("\n");
	/* ==================================================================
	 * SECTION 11: Long double tests
	 * ================================================================== */
	printf("--- Long double tests ---\n");
	{
		long double ld = 3.14159265358979323846L;
		TESTFMT("ld basic|3.141593|", "%Lf", ld);
		TESTFMT("ld sci|3.141593e+00|", "%Le", ld);
		TESTFMT("ld g|3.14159|", "%Lg", ld);
		TESTFMT("ld prec10|3.1415926536|", "%.10Lf", ld);
		TESTFMT("ld zero|0.000000|", "%Lf", 0.0L);
		TESTFMT("ld neg|-3.141593|", "%Lf", -ld);
		/* High-precision long double */
		long double pi_ld = 3.141592653589793238462643383279502884197L;
		TESTFMT("ld high prec|3.14159265358979323846|", "%.20Lf", pi_ld);
	}
	printf("\n");
	/* ==================================================================
	 * SECTION 12: Hexadecimal float tests (%a)
	 * ================================================================== */
	printf("--- Hex float %%a tests ---\n");
	{
		/* 1.0 = 0x1.0p+0 */
		TESTFMT("a 1.0|0x1p+0|", "%a", 1.0);
		/* 2.0 = 0x1.0p+1 */
		TESTFMT("a 2.0|0x1p+1|", "%a", 2.0);
		/* 0.5 = 0x1.0p-1 */
		TESTFMT("a 0.5|0x1p-1|", "%a", 0.5);
		/* 0.25 = 0x1.0p-2 */
		TESTFMT("a 0.25|0x1p-2|", "%a", 0.25);
		/* 3.0 = 0x1.8p+1 */
		TESTFMT("a 3.0|0x1.8p+1|", "%a", 3.0);
		/* 0.1 = 0x1.999999999999ap-4 */
		TESTFMT("a 0.1|0x1.999999999999ap-4|", "%a", 0.1);
		/* with precision */
		TESTFMT("a 1.0 prec2|0x1.00p+0|", "%.2a", 1.0);
		TESTFMT("a 3.14 prec|0x1.91eb851eb851fp+1|", "%.13a", 3.14);
	}
	printf("\n");
	/* ==================================================================
	 * SECTION 13: Edge cases
	 * ================================================================== */
	printf("--- Edge case tests ---\n");
	TESTFMT("very small|0.000000|", "%f", 1e-10);
	TESTFMT("very large|123456789012345680.000000|", "%f", 1.2345678901234568e17);
	TESTFMT("g trim trailing|1|", "%g", 1.0);
	TESTFMT("g trim trailing2|1.5|", "%g", 1.5);
	TESTFMT("g no trim with #|1.50000|", "%#5g", 1.5);
	TESTFMT("f huge prec|1.23456789012345679|", "%.17f", 1.23456789012345679);
	TESTFMT("e huge prec|1.23456789012345679e+00|", "%.17e", 1.23456789012345679);
	TESTFMT("empty format||", "");
	TESTFMT("text only|hello world|", "hello world");
	TESTFMT("mixed|abc42def|", "abc%ddef", 42);
	TESTFMT("multiple|1 2 3|", "%d %d %d", 1, 2, 3);
	printf("\n");
	/* ==================================================================
	 * SECTION 14: High-precision long double (conditional on math mode)
	 * ================================================================== */
	printf("--- High-precision long double (PI calculation) ---\n");
#define PI 3.141592653589793238462643383279502884197L
	{
		long double xx = 3.23454989171L - PI;
		long double result = (long double) (1.2345678902468e-13 * PI) / xx;
		char buf[128];
		int len = snprintf(buf, sizeof(buf), "%1.40Le", result);
		printf("  PI calc result (%d chars): %s\n", len, buf);
#if USE_LDOUBLE_MATH
		printf("  (Expected ~40 significant digits with GCC >= 16)\n");
#else
		printf("  (Expected ~17 significant digits with double fallback)\n");
#endif
	}

	/* ==================================================================
	 * SECTION 15: Integer alternate-form tests (%#hho and %#hhx)
	 * ================================================================== */
	printf("--- Integer alternate-form (%#hho and %#hhx) tests ---\n");
	{
	    unsigned char a;

	    /* Test %#hho (octal with alternate form) */
	    a = 0;
	    TESTFMT("hho zero|0|", "%#hho", a);

	    a = 1;
	    TESTFMT("hho one|01|", "%#hho", a);

	    a = 127;
	    TESTFMT("hho 127|0177|", "%#hho", a);

	    a = 255;
	    TESTFMT("hho 255|0377|", "%#hho", a);

	    /* Test %#hhx (hex with alternate form) */
	    a = 0;
	    TESTFMT("hhx zero|0|", "%#hhx", a);

	    a = 1;
	    TESTFMT("hhx one|0x1|", "%#hhx", a);

	    a = 127;
	    TESTFMT("hhx 127|0x7f|", "%#hhx", a);

	    a = 255;
	    TESTFMT("hhx 255|0xff|", "%#hhx", a);

	    /* Test with width and precision */
	    a = 42;
	    TESTFMT("hho width|       052|", "%#10hho", a);
	    TESTFMT("hhx width|      0x2a|", "%#10hhx", a);
	    TESTFMT("hho prec|00052|", "%#.5hho", a);
	    TESTFMT("hhx prec|0x02a|", "%#.5hhx", a);
	    TESTFMT("hho left|052       |", "%#-10hho", a);
	    TESTFMT("hhx left|0x2a      |", "%#-10hhx", a);
	}
	printf("\n");

	/* ==================================================================
	 * Additional
	 * ================================================================== */
	TESTFMT("f round dp|0.0010000|", "%.7f", 0.00099995);
	TESTFMT("f round int|10.0000000|", "%.7f", 9.99999995);
	TESTFMT("f small half|0.0000001|", "%.7f", 0.00000005);
	TESTFMT("f big half|123456790.0000000|", "%.7f", 123456789.99999995);
	TESTFMT("ld near1|1.00000000000000000000|", "%.20Lf", 0.99999999999999999995L);
	TESTFMT("ld near2|2.00000000000000000000|", "%.20Lf", 1.99999999999999999995L);
	TESTFMT("ld near05|0.50000000000000000000|", "%.20Lf", 0.49999999999999999995L);
	TESTFMT("ld dp|0.00100000000000000000|", "%.20Lf", 0.00099999999999999995L);
	TESTFMT("ld int|10.00000000000000000000|", "%.20Lf", 9.99999999999999999995L);
	TESTFMT("ld small|0.00000000000000000010|", "%.20Lf", 0.000000000000000000049L);
	TESTFMT("ld big|123456790.00000000000000000000|", "%.20Lf", 123456789.99999999999999999995L);
	TESTFMT("ld exp|1.23456789012345679000e+00|", "%.20Le", 1.23456789012345678995L);
	printf("\n");
	/* ==================================================================
	 * Summary
	 * ================================================================== */
	printf("=== Test Summary ===\n");
	printf("  Run:     %d\n", tests_run);
	/* Truncation, C99 7.19.6.5: the return value is the length the output
	 * would have had, the buffer gets size-1 characters and a NUL, and
	 * (NULL, 0) is the sizing idiom. Before the __SSTR case in OUT() every
	 * one of these returned -1. */
	{
		char tb[8];
		int n;
		memset(tb, 'x', sizeof(tb));
		n = snprintf(tb, sizeof(tb), "hello world");
		tests_run++;
		if (n == 11 && strcmp(tb, "hello w") == 0) { tests_passed++; printf("OK: truncation returns full length\n"); }
		else { tests_failed++; printf("FAIL: truncation returns full length: n=%d tb=\"%s\"\n", n, tb); }
		memset(tb, 'x', sizeof(tb));
		n = snprintf(tb, 0, "abc");
		tests_run++;
		if (n == 3 && tb[0] == 'x') { tests_passed++; printf("OK: size 0 writes nothing, returns length\n"); }
		else { tests_failed++; printf("FAIL: size 0: n=%d tb[0]=%c\n", n, tb[0]); }
		n = snprintf(NULL, 0, "%d/%s", 42, "xy");
		tests_run++;
		if (n == 5) { tests_passed++; printf("OK: (NULL, 0) sizing idiom\n"); }
		else { tests_failed++; printf("FAIL: (NULL, 0) sizing idiom: n=%d\n", n); }
		n = snprintf(tb, 7, "abcdef");
		tests_run++;
		if (n == 6 && strcmp(tb, "abcdef") == 0) { tests_passed++; printf("OK: exact fit\n"); }
		else { tests_failed++; printf("FAIL: exact fit: n=%d tb=\"%s\"\n", n, tb); }
	}

	printf("  Passed:  %d\n", tests_passed);
	printf("  Failed:  %d\n", tests_failed);
	printf("\n");
	return tests_failed > 0 ? 1 : 0;
}
#endif /* TESTME */
