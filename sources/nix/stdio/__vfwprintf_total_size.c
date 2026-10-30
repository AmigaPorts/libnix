/* ==========================================================================
 *  stdio vfwprintf.c
 *
 *  Minimal wide-char printf engine for AmigaOS / m68k (big-endian).
 *
 *  Conversions : d i o p u x X c s S n %
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
#include <wchar.h>
#include <wctype.h>

#include "ami_printf_math.h"

/* ======================================================================
 * SECTION: plumbing, flags, constants
 * ==================================================================== */

#ifdef FULL_SPECIFIERS
extern unsigned char *__decimalpoint;
#endif
extern int __vfwprintf_total_size(FILE *stream, const wchar_t *fmt, va_list args);

/* Output one wide character and handle stream errors.
 *
 * A string stream (the swprintf family) that is full keeps counting: C99
 * says the return value is the number of wide characters the output would
 * have had, and callers size a buffer with swprintf(NULL, 0, ...) on the
 * strength of that.  putwc on a full __SSTR stream returns WEOF from
 * __swbuf without touching the buffer, so only the count goes on.  WEOF
 * from a real stream is still an error.
 *
 * The count advances by one wide character per emitted character, matching
 * the character-count return value that swprintf expects.  It must NOT be
 * sizeof(wchar_t): that would return a byte count and break the sizing
 * idiom. */
#define OUT(c)  do { \
    if (putwc((c), stream) == WEOF && !(stream->_flags & __SSTR)) { \
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
 * Float-path buffer bounds (wide chars).  After the piece-list rework,
 * outbuffer only holds bounded content (sign, "0x", mantissa hex, digits,
 * L'.', exponent), and buffer[] holds only the digit string.  Nothing that
 * grows with width or precision is buffered any more.
 */
#define FLOAT_DIGBUF   (LD_DIGS + 6)                /* digit buffer for %f/%e/%g    */
#define FLOAT_OUTBUF   (FLOAT_DIGBUF + 24)          /* + sign, "0x", dot, exponent  */
#if FLOAT_OUTBUF < 128
# define FLOAT_OUTBUF_MIN 128
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
static const wchar_t flagc[] = { L'#', L'0', L'-', L' ', L'+' };
extern unsigned __ulldivus(unsigned long long *llp, unsigned short n);

/* A piece is a run of one repeated wide char, anchored at a wchar_t offset
 * in outbuffer.  Pieces are emitted strictly left to right, interleaved with
 * outbuffer content.  The array is kept sorted by startPos: the front pad
 * (if reserved) sits at index 0, value pieces follow in increasing order,
 * and a left-align tail pad (if any) is appended last. */
#define MAX_PIECES 8
struct piece {
    int     startPos;
    int     count;
    wchar_t c;
};

/* ======================================================================
 *  __vfwprintf_total_size -- the wide formatter core.
 *  Returns the number of wide characters written, or -1 on stream error.
 * ==================================================================== */
int __vfwprintf_total_size(FILE *stream, const wchar_t *format, va_list args) {
	unsigned long outcount = 0;
	__STDIO_LOCK(stream);
	while (*format) {
		/* ============================================================
		 * Ordinary characters: copy through unchanged.
		 * ========================================================== */
		if (*format != L'%') {
			OUT(*format++);
			continue;
		}
		/* ============================================================
		 * A '%' conversion begins here.
		 * ========================================================== */
		{
			static const wchar_t lowertabel[] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'a', L'b', L'c', L'd', L'e', L'f' };
			static const wchar_t uppertabel[] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };
			short width = 0;
			unsigned short preci = 0x7fff; /* Sentinel value: no precision was specified. */
			short flags = 0;
			wchar_t type;
			char orig_type;
			wchar_t subtype = L'i';
			wchar_t buffer1[4]; /* sign / radix prefix slot */
			wchar_t buffer[REQUIREDBUFFER + 8];
			wchar_t outbuffer[FLOAT_OUTBUF_MIN]; /* Float-path buffered output. */
#ifdef FULL_SPECIFIERS
			wchar_t *outp = outbuffer;
#endif
			wchar_t *buffer2 = buffer;
			unsigned long size1 = 0, size2 = 0;
			const wchar_t *ptr = format + 1;
			unsigned short i, pad;
			/* ---- 1. flags -------------------------------------------
			 * Scan flagc[] repeatedly until a full pass finds no match
			 * (loop condition becomes false when i reaches the size).
			 * --------------------------------------------------------- */
			do {
				for (i = 0; i < sizeof(flagc) / sizeof(flagc[0]); i++)
					if (flagc[i] == *ptr) {
						flags |= 1 << i;
						ptr++;
						break;
					}
			} while (i < sizeof(flagc) / sizeof(flagc[0]));
			/* ---- 2. field width: literal digits or '*' --------------- */
			if (*ptr == L'*') {
				signed int a;
				ptr++;
				a = va_arg(args, signed int);
				if (a < 0) { /* negative => left align   */
					flags |= LALIGNFLAG;
					width = -a;
				} else
					width = a;
			} else {
				while (iswdigit(*ptr))
					width = width * 10 + (*ptr++ - L'0');
			}
			/* ---- 3. precision: '.' digits or '.*' -------------------- */
			if (*ptr == L'.') {
				flags |= HAS_PRECI;
				ptr++;
				if (*ptr == L'*') /* read precision from arguments */
				{
					signed int a;
					ptr++;
					a = va_arg(args, signed int);
					if (a >= 0) /* negative means "unset"   */
						preci = a;
					else
						flags &= ~HAS_PRECI;
				} else {
					preci = 0;
					while (*ptr >= L'0' && *ptr <= L'9')
						preci = preci * 10 + (*ptr++ - L'0');
				}
			}
			/* ---- 4. length modifier ----------------------------------
			 * Internal codes: 'h' = halfword, 'i' = hh (byte),
			 * 'l' = long, 'm' = ll / intmax, 0 = none.
			 * --------------------------------------------------------- */
			if (*ptr == L'h' || *ptr == L'l' || *ptr == L'L' || *ptr == L'j' || *ptr == L'z' || *ptr == L't') {
				subtype = *ptr++;
				if (*ptr == L'h' || *ptr == L'l')
					++ptr, ++subtype;
			} else
				subtype = 0;
			orig_type = type = *ptr++;
			switch (type) {
			/* ========================================================
			 * Integer family: d i o p u x X
			 *======================================================== */
			case L'd':
			case L'i':
			case L'o':
			case L'p':
			case L'u':
			case L'x':
			case L'X': {
				unsigned long long v;
				const wchar_t *tabel;
				int base;
				/* %p is spelled "%#lx" */
				if (type == L'p') {
					subtype = L'l'; /* This is written as %#lx */
					type = L'x';
					flags |= ALTERNATEFLAG;
				}
				if (type == L'd' || type == L'i') {
					/* ---- signed: magnitude -> v, prefix -> buffer1 ---- */
					signed long long v2;
					/* Extract the argument with proper type */
					if (subtype == L'l')
						v2 = va_arg(args, signed long);
					else if (subtype == L'm' || subtype == L'j')
						v2 = va_arg(args, signed long long);
					else
						v2 = va_arg(args, signed int);
					/* Convert hh/h arguments to their specified width. */
					if (subtype == L'h')
						v2 = (short) v2; /* Convert to signed short instead of masking the bits. */
					else if (subtype == L'i')
						v2 = (signed char) v2; /* Convert to signed char instead of masking the bits. */
					/* Now v2 is correctly sign-extended */
					if (v2 < 0) {
						if (size1 < sizeof(buffer1) / sizeof(buffer1[0]))
							buffer1[size1++] = L'-';
						/* -LLONG_MIN cannot be negated in signed math    */
						v = (v2 == LLONG_MIN) ? (unsigned long long) LLONG_MAX + 1ULL : (unsigned long long) (-v2);
					} else {
						if (flags & SIGNFLAG)
							buffer1[size1++] = L'+';
						else if (flags & BLANKFLAG)
							buffer1[size1++] = L' ';
						v = (unsigned long long) v2;
					}
				} else {
					/* ---- unsigned family ---- */
					if (subtype == L'l')
						v = va_arg(args, unsigned long);
					else if (subtype == L'm' || subtype == L'j')
						v = va_arg(args, unsigned long long);
					else
						v = va_arg(args, unsigned int);
					/*
					 * For unsigned types, bitmasks are correct because we want
					 * to truncate to the specified width (0-255 for char, 0-65535 for short)
					 */
					if (subtype == L'h')
						v &= 0xffffULL;
					else if (subtype == L'i')
						v &= 0xffULL;
					if (flags & ALTERNATEFLAG) {
						if (type == L'o') {
							/*
							 * For octal with # flag:
							 * - If value is 0: just output "0" (no extra prefix)
							 * - If precision is specified and value is 0: don't add prefix
							 * - Otherwise add '0' prefix
							 */
							if (preci == 0x7fff && v != 0) {
								if (size1 < sizeof(buffer1) / sizeof(buffer1[0]))
									buffer1[size1++] = L'0';
							} else if (preci != 0x7fff && v != 0) {
								/* Precision specified and value non-zero: add prefix if needed */
								if (size1 < sizeof(buffer1) / sizeof(buffer1[0])) {
									buffer1[size1++] = L'0';
									if (preci > 0)									
										--preci;
								}
							}
							/* If v == 0, don't add prefix - just let the digit loop output "0" */
						} else if ((type == L'x' || type == L'X') && v) {
							if (size1 + 2 <= sizeof(buffer1) / sizeof(buffer1[0])) {
								buffer1[size1++] = L'0';
								buffer1[size1++] = type;
								if (preci != 0x7fff)
									preci -= 2;
							}
						}
					}
				}
				/* Divide down; digits land right-to-left in buffer.     */
				buffer2 = &buffer[sizeof(buffer) / sizeof(buffer[0])];
				base = (type == L'x' || type == L'X') ? 16 : (type == L'o') ? 8 : 10;
				tabel = (type != L'X') ? lowertabel : uppertabel;
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
			case L'c': {
				wchar_t ch;
				if (subtype == L'l')
					ch = (wchar_t) va_arg(args, long);
				else
					ch = (wchar_t) va_arg(args, int);
				buffer[0] = ch;
				buffer2 = buffer;
				size2 = 1;
				preci = 0;
				break;
			}
			/* ========================================================
			 * %s und %ls / %S  (NULL-safe)
			 *======================================================== */
			case L's':
			case L'S': {
				/* 'S' ->  %ls */
				if (type == L'S') {
					subtype = L'l';
				}

				if (subtype == L'l') {
					/* %ls -> wchar_t* */
					buffer2 = va_arg(args, wchar_t*);
					if (buffer2 == NULL) {
						buffer2 = L"(null)";
					}
					size2 = wcslen(buffer2);
					if (size2 > preci) {
						size2 = preci;
					}
				} else {
					/* %s -> char* */
					const char *s = va_arg(args, const char*);
					if (s == NULL) {
						buffer2 = L"(null)";
						size2 = 6;
					} else {
						/* limit size! */
						size_t max_len = (sizeof(outbuffer) / sizeof(wchar_t)) - 1;
						if (preci < max_len) {
							max_len = preci;
						}

						size2 = 0;
						/* convert */
						while (s[size2] != '\0' && size2 < max_len) {
							outbuffer[size2] = (wchar_t)(unsigned char)s[size2];
							size2++;
						}
						outbuffer[size2] = L'\0';
						buffer2 = outbuffer;
					}
					if (size2 > preci) {
						size2 = preci;
					}
				}
				preci = 0; /* Precision was already fully applied here */
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
			case L'a':
			case L'A':
			case L'f':
			case L'e':
			case L'E':
			case L'g':
			case L'G': {
				union _d_bits d;
				union _ld_bits lu; /* full-width ld operand */
				work_float_t ld_val; /* working precision     */
				int is_ld = (subtype == L'L');
				short exponent = 0; /* power of ten          */
				char sign = 0;
				const char *infnan = 0;
				wchar_t pad = ((flags & ZEROPADFLAG) && !(flags & LALIGNFLAG)) ? L'0' : L' ';
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
					int pad_at = (width > 0 && !(flags & LALIGNFLAG) &&
					              pad == '0' && sign != 0) ? 1 : 0;
					pieces[npieces++] = (struct piece){ .startPos = pad_at, .count = 0, .c = pad };
					if (preci == 0x7fff) {
						if (type == L'f' || type == L'F' || type == L'e' || type == L'E' || type == L'g' || type == L'G') {
							/* ISO C standard requires a default precision of 6 for floating-point formats */
							preci = 6;
						} else if (type == L'a' || type == L'A') {
							/* -1 signals the hex formatter to output enough digits for an exact value */
							preci = -1;
						} else {
							preci = 1;
						}
					}
					if (!infnan) {
						if (type != L'a' && type != L'A') {
							if (type == L'g' || type == L'G') {
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
							} else if (type == L'f' || type == L'F') {
								if (exponent >= 0) {
									digits_to_compute = exponent + preci + 1;
								} else {
									digits_to_compute = preci + exponent + 1;
									if (digits_to_compute < 0)
										digits_to_compute = 0;
								}
							} else if (type == L'e' || type == L'E') {
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
							buffer[0] = L'0';
							startPos = pos = 1;

							for (; pos <= digits_to_compute && pos < FLOAT_DIGBUF; ++pos) {
								buffer[pos] = L'0' + __LDL_mul_10_extract(&ld_val);
							}
							/* Perform rounding */
							if (__LDL_cmp_5(&ld_val) >= 0) {
								int tpos = pos - 1;
								do { // while (0);
									// even number
									if (0 == (buffer[tpos] & 1) && __LDL_cmp_5U(&ld_val) <= 0)
										break;

								while (tpos > 0) {
									if (++buffer[tpos] <= L'9')
										break;
									buffer[tpos--] = L'0';
								}
								/* Handle carry overflow (e.g., 9.99 -> 10.00 or 0.099 -> 0.100) */
								if (tpos == 0) {
									startPos = 0;
									buffer[0] = L'1';
									exponent++; /* Exponent shifts up globally for all format paths */
									if (exponent > 0) {
										++leading;
										++ digits_to_compute;
									}
								} else if (exponent + preci < 0 && buffer[tpos] == L'5') {
									startPos = 0;
									buffer[0] = L'1';
									exponent++; /* Exponent shifts up globally for all format paths */
								}
								} while (0); // end do
							}
							/* Set the number of integer digits; this matters only for %f. */
							if (type == L'f' || type == L'F') {
								leading = (exponent >= 0) ? (exponent + 1) : 0;
							} else {
								leading = 1;
							}
						} else {
							/* 'a' 'A' format - pure bit operations */
							if (!(flags & HAS_PRECI))
								killZero = 1;
							TOUT(L'0');
							TOUT(type + (L'X' - L'A'));
							buffer[0] = L'0';
							startPos = 0;
							pos = 1;
							const wchar_t *hexdigits = (type == L'A') ? L"0123456789ABCDEF" : L"0123456789abcdef";
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
									buffer[0] = L'1';
								}
								/* Extract mantissa for long double */
								unsigned long long mant = lu.b.mant;
								/* Drop the explicit integer bit (bit 63) */
								mant &= 0x7FFFFFFFFFFFFFFFULL;
								/* Extract 16 hex digits (64 bits) */
								for (int i = 0; i < 16; i++) {
								    int digit = (mant >> (60 - 4*i)) & 0xF;
								    buffer[pos++] = hexdigits[digit];
								}
							} else {
								if (!d.b.exp) {
									exponent = 0;
								} else {
									exponent = d.b.exp - 1023;
									buffer[0] = L'1';
								}
								/* Extract 52-bit mantissa for double */
								unsigned long long mant = ((unsigned long long)d.b.frac0 << 32) | d.b.frac1;
								/* Drop implicit integer bit */
								mant &= 0xFFFFFFFFFFFFFULL;
								/* Print 13 hex digits (52 bits) */
								for (int i = 0; i < 13; i++) {
								    int digit = (mant >> (48 - 4*i)) & 0xF;
								    buffer[pos++] = hexdigits[digit];
								}
							}
							type += L'P' - L'A';
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
					 * (sign, "0x", digits, L'.', exponent) ever goes into
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
							pieces[0].c = L' ';
					} else {
						unsigned short frac = preci;

						/* 2. Integer part. */
						if (exponent < 0 && (type == L'f' || type == L'F')) {
							/* For small fixed-point numbers (e.g., 0.00123), print the mandatory leading '0' */
							TOUT(L'0');
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
								PUSH((int)(outp - outbuffer), leading, L'0');
							if (type == L'f' || type == L'F') {
								if (rpad > 0)
									PUSH((int)(outp - outbuffer), rpad, L'0');
							}
						}
						/* 3. Trim trailing zeros in the digit buffer (g/G). */
						if (killZero) {
							int stop = pos - 1;
							while (stop >= startPos && buffer[stop] == L'0') {
								stop--;
							}
							preci = stop - startPos + 1;   /* Number of significant digits. */
							if (preci > 0x3fff)
								preci = 0;
							pos = stop + 1;
						}
						/* 4. Decimal point. */
						if (preci || (flags & ALTERNATEFLAG) != 0) {
							TOUT((__decimalpoint && __decimalpoint[0]) ? (wchar_t)__decimalpoint[0] : L'.');
						}
						/* limit output */
						if (pos > startPos + preci) {
							pos = startPos + preci;
						}
						/* 5. Leading fractional zeros for small %f numbers. */
						if (exponent < 0 && (type == L'f' || type == L'F')) {
							int fractional_zeros = -exponent - 1; /* e.g., exp = -3 -> 2 zeros */
							if (fractional_zeros > frac)
								fractional_zeros = frac;
							if (fractional_zeros > 0) {
								PUSH((int)(outp - outbuffer), fractional_zeros, L'0');
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
							PUSH((int)(outp - outbuffer), preci, L'0');
							preci = 0;
						}
						/* 8. Exponent. */
						if (!(type == L'f' || type == L'F')) {
							int xout = 0;
							wchar_t t_lower = (wchar_t) (type | 0x20); /* Convert to lowercase ('e' or 'p') */
							TOUT(type);
							if (exponent < 0) {
								TOUT(L'-');
								exponent = -exponent;
							} else {
								TOUT(L'+');
							}
							/* Thousands digit (e.g., for long double extremes like e+4932) */
							if (exponent > 999) {
								int z = exponent / 1000;
								TOUT(L'0' + z);
								exponent -= z * 1000;
								xout = 1;
							}
							/* Hundreds digit */
							if (xout || exponent > 99) {
								int z = exponent / 100;
								TOUT(L'0' + z);
								exponent -= z * 100;
								xout = 1;
							}
							/* Tens digit: Forced to print for 'e'/'E' to guarantee at least 2 digits (e+00) */
							if (xout || exponent > 9 || t_lower == L'e') {
								int z = exponent / 10;
								TOUT(L'0' + z);
								exponent -= z * 10;
							}
							/* Units digit (always printed) */
							TOUT(L'0' + exponent);
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
								PUSH((int)(outp - outbuffer), padcount, L' ');
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
								wchar_t pc = pieces[pi].c;
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
							wchar_t pc = pieces[pi].c;
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
			case L'%':
				buffer2 = L"%";
				size2 = 1;
				preci = 0;
				break;
			case L'n':
				*va_arg(args, int*) = outcount;
				width = preci = 0;
				break;
			default:
				if (!type)
					ptr--; /* We've gone too far - step one back */
				buffer2 = (wchar_t*) format;
				size2 = ptr - format;
				width = preci = 0;
				break;
			}
			if ((flags & HAS_PRECI) && !(flags & ALTERNATEFLAG)) {
				if (size2 == 1 && !preci && buffer2[0] == L'0')
					size2 = 0;
			}
			if ((flags & HAS_PRECI) && !preci) {
			    if (size2 == 1 && buffer2[0] == '0' &&
			        (orig_type == 'd' || orig_type == 'i' ||
			         orig_type == 'x' || orig_type == 'X'))
			        size2 = 0;
			}
			pad = size1 + (size2 >= preci ? size2 : preci); /* Calculate the number of characters */
			pad = pad >= width ? 0 : width - pad; /* and the number of resulting pad bytes */
			if (flags & ZEROPADFLAG) /* Print the sign and prefix. */
				for (i = 0; i < size1; i++)
					OUT(buffer1[i]);
			if (!(flags & LALIGNFLAG)) /* Pad left */
				for (i = 0; i < pad; i++)
					OUT(flags & ZEROPADFLAG ? L'0' : L' ');
			if (!(flags & ZEROPADFLAG)) /* Print the sign when zero padding is not used. */
				for (i = 0; i < size1; i++)
					OUT(buffer1[i]);
			for (i = size2; i < preci; i++) /* extend to precision */
				OUT(L'0');
			for (i = 0; i < size2; i++) /* Print the converted value. */
				OUT(buffer2[i]);
			if (flags & LALIGNFLAG) /* Pad right */
				for (i = 0; i < pad; i++)
					OUT(L' ');
			format = ptr;
		}
	}
	__STDIO_UNLOCK(stream);
	return outcount;
}
