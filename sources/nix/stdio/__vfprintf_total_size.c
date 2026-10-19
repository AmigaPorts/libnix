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
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <float.h>
#include <stdio.h>

#include "ami_printf_math.h"

/* ======================================================================
 * SECTION: plumbing, flags, constants
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

/*
 * Float-path buffer bounds.  After the piece-list rework, outbuffer only ever
 * holds bounded content (sign, "0x", mantissa hex, digits, '.', exponent),
 * and buffer[] holds only the digit string.  Nothing that grows with width or
 * precision is buffered any more, so these can be small.
 */
#define FLOAT_DIGBUF   (LD_DIGS + 6)                /* digit buffer for %f/%e/%g    */
#define FLOAT_OUTBUF   (FLOAT_DIGBUF + 24)          /* + sign, "0x", dot, exponent  */
#if FLOAT_OUTBUF < 64
# define FLOAT_OUTBUF_MIN 64
#else
# define FLOAT_OUTBUF_MIN FLOAT_OUTBUF
#endif

/* Flag bits; index in flagc[] == bit number.                           */
#define ALTERNATEFLAG  1        /* '#'   */
#define ZEROPADFLAG    2        /* '0'   */
#define LALIGNFLAG     4        /* '-'   */
#define BLANKFLAG      8        /* ' '   */
#define SIGNFLAG       16       /* '+'   */
#define HAS_PRECI      32       /* '.' seen */
static const char flagc[] = { '#', '0', '-', ' ', '+' };
extern unsigned __ulldivus(unsigned long long *llp, unsigned short n);

/* A piece is a run of one repeated char, anchored at a byte offset in
 * outbuffer.  Pieces are emitted strictly left to right, interleaved with
 * outbuffer content.  The array is kept sorted by startPos: the front pad
 * (if reserved) sits at index 0, value pieces follow in increasing order,
 * and a left-align tail pad (if any) is appended last. */
#define MAX_PIECES 8
struct piece {
    int  startPos;
    int  count;
    char c;
};

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
			char outbuffer[FLOAT_OUTBUF_MIN]; /* Float-path buffered output. */
			char *outp = outbuffer;
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
				 *   S6  padded emission (piece-list interleave)
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
                const int MEANINGFUL_DIGITS = is_ld ? LD_DIGS : D_DIGS;
				/* -------- S1: load + make magnitude + specials ------- */
				if (is_ld) {
					long double raw_ld = va_arg(args, long double);
					lu.ld = raw_ld;
                    ld_val = __LDL_from_long_double(raw_ld);   /* ignore sign*/
					if (_ld_signbit(lu)) {
						sign = '-';
						lu.b.sign = 0; /* erase sign field      */
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
					ld_val = __LDL_from_double(d.d);
					if (_d_signbit(d)) {
						d.b.sign = 0;
						sign = '-';
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
							normalized_exp = (int) ((e * LOG10_2_Q32 + (1ULL << 31)) >> 32);
							if (bit_exp < 0)
								normalized_exp = -normalized_exp;
						}
						/* Use pow10_ld() for accurate initial scaling */
					    ld_val = pow10_ld(ld_val, -normalized_exp);

						/* Fine-tune to [1,10) with minimal iterations */
						while (__LDL_cmp_10(&ld_val) > 0 && normalized_exp < 5000) {
						    ld_val = __LDL_mul(ld_val, WORK_0_1);
						    ++normalized_exp;
						}

						while (__LDL_cmp_1(&ld_val) < 0 && normalized_exp > -5000) {
						    ld_val = __LDL_mul_10(ld_val);
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
					/* Piece list for this conversion.  Index 0 may be the
					 * reserved front pad (right-align only); value pieces
					 * follow in increasing startPos order; a left-align
					 * tail pad is appended at step 9. */
					struct piece pieces[MAX_PIECES];
					int npieces = 0;
#define PUSH(at, cnt, ch) do { \
    if ((cnt) > 0 && npieces < MAX_PIECES) \
        pieces[npieces++] = (struct piece){ .startPos = (at), .count = (cnt), .c = (ch) }; \
} while (0)

					/* 0. Reserve the front pad slot (always index 0) when
					 *    the pad goes before the value.  Its count is set
					 *    at step 9. */
					if (width > 0 && !(flags & LALIGNFLAG)) {
						int pad_at = (pad == '0' && sign != 0) ? 1 : 0;
						pieces[npieces++] = (struct piece){ .startPos = pad_at, .count = 0, .c = pad };
					}

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
									digits_to_compute = preci + exponent + 1;
									if (digits_to_compute < 0)
										digits_to_compute = 0;
								}
							} else if (type == 'e' || type == 'E') {
								digits_to_compute = preci + 1;
								if (digits_to_compute > MEANINGFUL_DIGITS)
									digits_to_compute = MEANINGFUL_DIGITS;
							} else {
								digits_to_compute = preci + 2;
							}
							if (digits_to_compute < 0)
								digits_to_compute = 0;
							else if (digits_to_compute > MEANINGFUL_DIGITS) {
								digits_to_compute = MEANINGFUL_DIGITS;
							}
							/* Use ld_val for digit extraction, not d.d */
							buffer[0] = '0';
							startPos = pos = 1;

							for (; pos <= digits_to_compute && pos < FLOAT_DIGBUF; ++pos) {
								buffer[pos] = '0' + __LDL_mul_10_extract(&ld_val);
							}
							/* Perform rounding */
							if (__LDL_cmp_5(&ld_val) >= 0) {
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
					/*
					 * Build a tiny piece list of unbounded runs (zeros / pad),
					 * anchored at outbuffer offsets, then emit outbuffer
					 * interleaved with those runs.  Only bounded content
					 * (sign, "0x", digits, '.', exponent) ever goes into
					 * outbuffer, so any width or precision is safe.
					 */
					/* 1. Sign (bounded; stays in outbuffer[0]). */
					if (sign != 0) {
						TOUT(sign);
					}
					if (infnan) {
						/* Specials are tiny; just emit inline.  The 0 flag
						 * is ignored for inf/NaN, so the front pad (if any)
						 * uses spaces. */
						TOUT(infnan[0]);
						TOUT(infnan[1]);
						TOUT(infnan[2]);
						if (width > 0 && !(flags & LALIGNFLAG))
							pieces[0].c = ' ';
					} else {
						/* 2. Integer part. */
						if (exponent < 0 && (type == 'f' || type == 'F')) {
							/* For small fixed-point numbers (e.g., 0.00123), print the mandatory leading '0' */
							TOUT('0');
						} else {
							if (isg)
								preci -= leading;
							int rpad = exponent - leading;
							int int_start = startPos;
							while (startPos < pos && leading-- > 0) {
								if (isg && !(flags & ALTERNATEFLAG)) {
									--preci;
									if (preci > 0x3fff)
										preci = 0;
								}
								++startPos;
							}
							/* Copy the integer digit span into outbuffer. */
							for (int k = int_start; k < startPos; ++k)
								TOUT(buffer[k]);
							/* Remaining integer zeros go into a piece. */
							if (leading > 0)
								PUSH((int)(outp - outbuffer), leading, '0');
							if (type == 'f' || type == 'F') {
								if (rpad > 0)
									PUSH((int)(outp - outbuffer), rpad, '0');
							}
						}
						/* 3. Trim trailing zeros in the digit buffer (g/G). */
						if (killZero) {
							int stop = pos - 1;
							while (stop >= startPos && buffer[stop] == '0') {
								stop--;
							}
							preci = stop - startPos + 1;   /* Number of significant digits. */
							if (preci > 0x3fff)
								preci = 0;
							pos = stop + 1;
						}
						/* 4. Decimal point. */
						if (preci || (flags & ALTERNATEFLAG) != 0) {
							TOUT((__decimalpoint && __decimalpoint[0]) ? __decimalpoint[0] : '.');
						}
						/* limit output */
						if (pos > startPos + preci) {
							pos = startPos + preci;
						}
						/* 5. Leading fractional zeros for small %f numbers. */
						if (exponent < 0 && (type == 'f' || type == 'F')) {
							int fractional_zeros = -exponent - 1; /* e.g., exp = -3 -> 2 zeros */
							if (fractional_zeros > preci)
								fractional_zeros = preci;
							if (fractional_zeros > 0) {
								PUSH((int)(outp - outbuffer), fractional_zeros, '0');
								if (!isg)
									preci -= fractional_zeros;
							}
						}
						/* 6. Significant fractional digits from the buffer. */
						{
							int nfrac = pos - startPos;
							for (int k = 0; k < nfrac; ++k)
								TOUT(buffer[startPos + k]);
							preci -= nfrac;
						}
						/* 7. Trailing zeros to satisfy precision. */
						if (preci > 0) {
							PUSH((int)(outp - outbuffer), preci, '0');
							preci = 0;
						}
						/* 8. Exponent. */
						if (!(type == 'f' || type == 'F')) {
							int xout = 0;
							char t_lower = (char) (type | 0x20); /* Convert to lowercase ('e' or 'p') */
							TOUT(type);
							if (exponent < 0) {
								TOUT('-');
								exponent = -exponent;
							} else {
								TOUT('+');
							}
							/* Thousands digit (e.g., for long double extremes like e+4932) */
							if (exponent > 999) {
								int z = exponent / 1000;
								TOUT('0' + z);
								exponent -= z * 1000;
								xout = 1;
							}
							/* Hundreds digit */
							if (xout || exponent > 99) {
								int z = exponent / 100;
								TOUT('0' + z);
								exponent -= z * 100;
								xout = 1;
							}
							/* Tens digit: Forced to print for 'e'/'E' to guarantee at least 2 digits (e+00) */
							if (xout || exponent > 9 || t_lower == 'e') {
								int z = exponent / 10;
								TOUT('0' + z);
								exponent -= z * 10;
							}
							/* Units digit (always printed) */
							TOUT('0' + exponent);
						}
					}
					/* 9. Fill in the pad count.
					 *
					 * width is the requested field width.  vlen is the
					 * number of characters the value itself contributes
					 * (everything already in outbuffer, plus the runs queued
					 * as pieces -- including the reserved front pad, whose
					 * count is still 0 at this point).  padcount is the
					 * number of pad characters actually needed, clamped at 0.
					 */
					{
						int vlen = (int)(outp - outbuffer);
						int padcount;
						int pi;

						for (pi = 0; pi < npieces; ++pi)
							vlen += pieces[pi].count;

						padcount = (width > vlen) ? (width - vlen) : 0;

						if (padcount > 0) {
							if (flags & LALIGNFLAG) {
								/* Tail pad: append at the end so its
								 * startPos (== outp) is the largest. */
								PUSH((int)(outp - outbuffer), padcount, ' ');
							} else {
								/* Front pad: the slot at index 0 was
								 * reserved.  padcount > 0 implies
								 * width > 0, and we are in the non-
								 * LALIGNFLAG branch, so the slot is
								 * guaranteed to exist. */
								pieces[0].count = padcount;
							}
						}
					}
					/* 10. Emit: interleave outbuffer with pieces. */
					{
						int len = (int)(outp - outbuffer);
						int pi = 0;
						for (int k = 0; k < len; ++k) {
							while (pi < npieces && pieces[pi].startPos == k) {
								int cnt = pieces[pi].count;
								char pc = pieces[pi].c;
								for (int j = 0; j < cnt; ++j)
									OUT(pc);
								++pi;
							}
							OUT(outbuffer[k]);
						}
						/* Pieces anchored at the end (left-align tail,
						 * or width pad when outbuffer is empty). */
						while (pi < npieces) {
							int cnt = pieces[pi].count;
							char pc = pieces[pi].c;
							for (int j = 0; j < cnt; ++j)
								OUT(pc);
							++pi;
						}
					}
#undef PUSH
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
