/* ----------------------------------------------------------------------
 * scanf_impl.h -- shared body of vfscanf / vfwscanf
 *
 * Compile with -DWIDE for the wchar_t version, without it for the
 * char version.  Define VSCANF_NAME to the exported function name
 * (vfscanf or vfwscanf) before including this header.
 * -------------------------------------------------------------------- */
#ifndef __SCANF_IMPL_H
#define __SCANF_IMPL_H

#ifdef WIDE

# include "wchar.h"
# include "wctype.h"

# define TCHAR          wchar_t
# define GETC(s)        getwc(s)
# define UNGETC(c,s)    ungetwc((c), (s))
# define ISDIGIT(c)     iswdigit(c)
# define ISXDIGIT(c)    iswxdigit(c)
# define ISSPACE(c)     iswspace(c)
# define TOLOWER(c)     towlower(c)
# define TOUPPER(c)     towupper(c)

# define CX_0           L'0'
# define CX_1           L'1'
# define CX_5           L'5'
# define CX_7           L'7'
# define CX_9           L'9'
# define CX_a           L'a'
# define CX_A           L'A'
# define CX_c           L'c'
# define CX_d           L'd'
# define CX_e           L'e'
# define CX_E           L'E'
# define CX_f           L'f'
# define CX_F           L'F'
# define CX_g           L'g'
# define CX_G           L'G'
# define CX_h           L'h'
# define CX_i           L'i'
# define CX_j           L'j'
# define CX_l           L'l'
# define CX_L           L'L'
# define CX_m           L'm'
# define CX_n           L'n'
# define CX_o           L'o'
# define CX_p           L'p'
# define CX_P           L'P'
# define CX_s           L's'
# define CX_u           L'u'
# define CX_x           L'x'
# define CX_X           L'X'
# define CX_dot         L'.'
# define CX_minus       L'-'
# define CX_plus        L'+'
# define CX_space       L' '
# define CX_percent     L'%'
# define CX_bracket     L'['
# define CX_rbracket    L']'
# define CX_star        L'*'
# define CX_caret       L'^'
# define CX_NUL         L'\0'

# define CX_EOF         WEOF

#define __decimalpoint __wcdecimalpoint
static wchar_t * __wcdecimalpoint = L".";

#else /* narrow */

# define TCHAR          char
# define GETC(s)        getc(s)
# define UNGETC(c,s)    ungetc((c), (s))
# define ISDIGIT(c)     isdigit(c)
# define ISXDIGIT(c)    isxdigit(c)
# define ISSPACE(c)     isspace(c)
# define TOLOWER(c)     tolower(c)
# define TOUPPER(c)     toupper(c)

# define CX_0           '0'
# define CX_1           '1'
# define CX_5           '5'
# define CX_7           '7'
# define CX_9           '9'
# define CX_a           'a'
# define CX_A           'A'
# define CX_c           'c'
# define CX_d           'd'
# define CX_e           'e'
# define CX_E           'E'
# define CX_f           'f'
# define CX_F           'F'
# define CX_g           'g'
# define CX_G           'G'
# define CX_h           'h'
# define CX_i           'i'
# define CX_j           'j'
# define CX_l           'l'
# define CX_L           'L'
# define CX_m           'm'
# define CX_n           'n'
# define CX_o           'o'
# define CX_p           'p'
# define CX_P           'P'
# define CX_s           's'
# define CX_u           'u'
# define CX_x           'x'
# define CX_X           'X'
# define CX_dot         '.'
# define CX_minus       '-'
# define CX_plus        '+'
# define CX_space       ' '
# define CX_percent     '%'
# define CX_bracket     '['
# define CX_rbracket    ']'
# define CX_star        '*'
# define CX_caret       '^'
# define CX_NUL         '\0'

# define CX_EOF         EOF

#endif

/* __decimalpoint is provided by the library.  The narrow one is
 * unsigned char *, the wide one must be wchar_t * (libnix declares
 * it as such for the wide build). */
#ifdef FULL_SPECIFIERS
#include "../../math/stdio/ami_printf_math.h"

static int __LDL_bin_exp(const LDL *x)
{
    if (x->l.high)
        return 95 - __builtin_clz(x->l.high);
    if (x->l.middle)
        return 63 - __builtin_clz(x->l.middle);
    if (x->l.low)
        return 31 - __builtin_clz(x->l.low);
    return 0;
}

extern TCHAR * __decimalpoint;
#endif

/* some macros to cut this short
 * NEXT(c);     read next character
 * PREV(c);     ungetc a character
 * VAL(a)       leads to 1 if a is true and valid
 */

#define NEXT(c) do { \
    (c) = GETC(stream); \
    if ((c) != CX_EOF) { \
        size++; \
        incount++; \
    } \
} while(0)

#define PREV(c) do { \
    if ((c) != CX_EOF) { \
        UNGETC((c), stream); \
        size--; \
        incount--; \
    } \
} while(0)

#define VAL(a)  ((a)&&size<=width)

static inline int hexval(int c)
{
	if (c >= CX_0 && c <= CX_9)
		return c - CX_0;
	if (c >= CX_a && c <= CX_f)
		return c - CX_a + 10;
	if (c >= CX_A && c <= CX_F)
		return c - CX_A + 10;
	return 0;
}

#ifdef FULL_SPECIFIERS
static const unsigned char undef[3][sizeof(double)] = /* Undefined numeric values, IEEE */
{ { 0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* +inf */
{ 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* -inf */
{ 0x7f, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } /*  NaN */
};
#endif

int VSCANF_NAME(FILE *stream, const TCHAR *format, va_list args) {
	__STDIO_LOCK(stream);
	size_t blocks = 0, incount = 0;
	int c = 0;
	/* EOF only for an input failure before the first conversion, even one
	 that assigns nothing (%*d) */
	int converted = 0;

	while (*format) {
		size_t size = 0;

		if (*format == CX_percent) {
			size_t width = ULONG_MAX;
			TCHAR type, subtype = CX_i, ignore = 0;
			const TCHAR *ptr = format + 1;
			size_t i;

			if (ISDIGIT(*ptr)) {
				width = 0;
				while (ISDIGIT(*ptr))
					width = width * 10 + (*ptr++ - CX_0);
			}

			while (*ptr == CX_h || *ptr == CX_l || *ptr == CX_L || *ptr == CX_star) {
				if (*ptr == CX_star)
					ignore = 1;
				else
					subtype = *ptr;
				ptr++;
			}

			type = *ptr++;

			if (type && type != CX_percent && type != CX_c && type != CX_n
					&& type != CX_bracket) {
				do /* ignore leading whitespace characters */
					NEXT(c); while (ISSPACE(c));
				if (c == CX_EOF)
					break;
				size = 1;
			} /* The first non-whitespace character is already read */

			switch (type) {
			case CX_c: {
				TCHAR *bp;

				if (width == ULONG_MAX) /* Default */
					width = 1;

if (!ignore)
					bp = va_arg(args, TCHAR *);
				else
					bp = NULL; /* Just to get the compiler happy */

												NEXT(c); /* 'c' did not skip whitespace */
				while (VAL(c != CX_EOF)) {
					if (!ignore)
						*bp++ = (TCHAR) c;
					NEXT(c);
				}
				PREV(c);

				if (!ignore && size == width)
					blocks++;
				break;
			}
			case CX_bracket: {
				TCHAR *bp;
				unsigned char tab[32], a, b;
				char circflag = 0;

				if (*ptr == CX_caret) {
					circflag = 1;
					ptr++;
				}
				for (i = 0; i < sizeof(tab); i++)
					tab[i] = circflag ? 255 : 0;
				for (;;) {
					if (!*ptr)
						break;
					a = b = (unsigned char) *ptr++;
					if (*ptr == CX_minus && ptr[1] && ptr[1] != CX_rbracket) {
						ptr++;
						b = (unsigned char) *ptr++;
					}
					for (i = a; i <= b; i++)
						if (circflag)
							tab[i / 8] &= ~(1 << (i & 7));
						else
							tab[i / 8] |= 1 << (i & 7);
					if (*ptr == CX_rbracket) {
						ptr++;
						break;
					}
				}

				if (!ignore)
					bp = va_arg(args, TCHAR *);
				else
					bp = NULL; /* Just to get the compiler happy */

												NEXT(c);
				while (VAL(c != CX_EOF && c >= 0 && c < 256 &&
						(tab[c/8] & (1<<(c&7))))) {
					if (!ignore)
						*bp++ = (TCHAR) c;
					NEXT(c);
				}
				PREV(c);

				if (!ignore && size) {
					*bp++ = CX_NUL;
					blocks++;
				}
				break;
			}
			case CX_s: {
				TCHAR *bp;

				if (!ignore)
				bp = va_arg(args, TCHAR *);
				else
				bp = NULL; /* Just to get the compiler happy */

				while (VAL(c != CX_EOF && !ISSPACE(c))) {
					if (!ignore)
						*bp++ = (TCHAR) c;
					NEXT(c);
				}
				PREV(c);

				if (!ignore && size) {
					*bp++ = CX_NUL;
					blocks++;
				}
				break;
			}
#ifdef FULL_SPECIFIERS
			case CX_e:
			case CX_f:
			case CX_g: {
			    LDL lv = { .l = { 0, 0, 0, 0 } };
			    /* dec_exp maintains the invariant: true value = lv * 10^dec_exp.
			     * Updated locally at each absorption decision:
			     *   integer digit absorbed:              unchanged
			     *   integer digit rejected (saturated):  +1  (true value 10x larger)
			     *   fractional digit absorbed:           -1  (one more decimal place in lv)
			     *   fractional digit rejected:           unchanged (below 96-bit precision)
			     *   explicit e/E exponent:               += signed exponent (applied once)
			     */
			    int dec_exp = 0;
			    int leading_digit = 0;
			    int min = 0;
			    int sign = 1;
			    int has_digit = 0;
			    int is_L = (subtype == CX_L);
			    int special = 0;   /* 0 = finite, 1 = inf, 2 = nan, 3 = +/-0.0 */
			    int bin_exp = 0;
			    int has_non_zero = 0;

			    if (VAL(c == CX_minus || c == CX_plus)) {
			        min = c;
			        sign = (c == CX_minus) ? -1 : 1;
			        NEXT(c);
			    }

			    /* inf / nan */
			    if (VAL(TOLOWER(c) == CX_i)) {
			        int d;
			        NEXT(d);
			        if (VAL(TOLOWER(d) == CX_n)) {
			            int e;
			            NEXT(e);
			            if (VAL(TOLOWER(e) == CX_f)) {
			                special = 1;
			                has_digit = 1;
			                goto float_done;
			            }
			            PREV(e);
			        }
			        PREV(d);
			    } else if (VAL(TOLOWER(c) == CX_n)) {
			        int d;
			        NEXT(d);
			        if (VAL(TOLOWER(d) == CX_a)) {
			            int e;
			            NEXT(e);
			            if (VAL(TOLOWER(e) == CX_n)) {
			                special = 2;
			                has_digit = 1;
			                goto float_done;
			            }
			            PREV(e);
			        }
			        PREV(d);
			    }

			    /* Integer part.  __LDL_mul_10_add returns 0 when the
			     * accumulator is saturated; the digit is then not represented
			     * in lv, so the true value is 10x larger than lv shows:
			     * dec_exp += 1.  Keep consuming digits so the decimal point
			     * position and the explicit exponent stay correct. */
			    while (VAL(ISDIGIT(c))) {
			        int v = c - CX_0;
			        if (!__LDL_mul_10_add(&lv, v))
			            ++dec_exp;          /* dropped integer digit: value 10x larger */
			        has_digit = 1;
			        if (!leading_digit)
			            leading_digit = v;
			        NEXT(c);
			    }

			    /* Hex float path: only when integer part was exactly "0" */
			    if (has_digit && VAL(c == CX_x) &&
			        lv.l.high == 0 && lv.l.middle == 0 && lv.l.low == 0) {
			        int has_hex = 0;

			        NEXT(c);   /* consume 'x' */

			        /* Integer hex digits.  Every accepted digit contributes
			         * 4 bits of value; once the accumulator saturates, the
			         * digit's own bit-length determines how much the value
			         * grows (all subsequent digits are below the LSB, so
			         * they're worth 4 bits each). */
			        while (VAL(ISXDIGIT(c))) {
			            int v = hexval(c);
			            if (!__LDL_mul_16_add(&lv, v))
			                bin_exp += 4;
			            has_non_zero |= v;
			            has_hex = 1;
			            has_digit = 1;
			            NEXT(c);
			        }

			        /* Fold in the bit-length of the accumulated mantissa. */
			        bin_exp += __LDL_bin_exp(&lv);

			        /* Fractional hex digits.  Only accumulated digits
			         * contribute to bin_exp; skipped ones are below the
			         * 96-bit precision and are noise. */
			        if (VAL(c == __decimalpoint[0])) {
			            NEXT(c);
			            if (!has_non_zero)
			                bin_exp = -1;
			            while (VAL(ISXDIGIT(c))) {
			                int v = hexval(c);
			                if (__LDL_mul_16_add(&lv, v) && !has_non_zero) {
			                    if (v == 0)
			                        bin_exp -= 4;
			                    else if (v == 1)
			                        bin_exp -= 3;
			                    else if (v <= 3)
			                        bin_exp -= 2;
			                    else if (v <= 7)
			                        bin_exp -= 1;
			                }
			                has_non_zero |= v;
			                has_hex = 1;
			                has_digit = 1;
			                NEXT(c);
			            }
			        }

			        if (!has_hex) {
			            /* "0x" with no hex digits: matching failure. */
			            PREV(c);
			            has_digit = 0;
			            size = 0;
			            goto float_done;
			        }

			        __LDL_normalize(&lv);

			        if (VAL(TOLOWER(c) == CX_p)) {
			            int d;
			            int pexp = 0;
			            int psign_char = 0;
			            int has_pexp = 0;

			            NEXT(d);
			            if (VAL(d == CX_minus || d == CX_plus)) {
			                psign_char = d;
			                NEXT(d);
			            }
			            while (VAL(ISDIGIT(d))) {
			                pexp = pexp * 10 + (d - CX_0);
			                if (pexp > 16383)
			                    pexp = 16383; /* large enough */
			                has_pexp = 1;
			                NEXT(d);
			            }
			            PREV(d);
			            if (!has_pexp) {
			                /* "0x1p" or "0x1p+": matching failure. */
			                has_digit = 0;
			                size = 0;
			            } else {
			                bin_exp += (psign_char == CX_minus) ? -pexp : pexp;
			            }
			        } else {
			            PREV(c);
			        }
			        lv.l.exp += bin_exp;


			        goto float_done;
			    }

			    /* Decimal fractional part.
			     * Each absorbed fractional digit does lv = lv*10 + d, which
			     * appends one decimal place to lv's integer interpretation;
			     * dec_exp -= 1 restores it.  Zeros count: appending 0 still
			     * multiplies lv by 10 and shifts the implied decimal point.
			     * A digit rejected by __LDL_mul_10_add (lv saturated) is not
			     * represented in lv, so it must NOT touch dec_exp. */
			    if (VAL(c == __decimalpoint[0])) {
			        NEXT(c);
			        while (VAL(ISDIGIT(c))) {
			            int v = c - CX_0;
			            if (__LDL_mul_10_add(&lv, v))
			                --dec_exp;      /* absorbed: one more decimal place in lv */
			            if (!leading_digit)
			                leading_digit = v;
			            has_digit = 1;
			            NEXT(c);
			        }
			    }

			    if (!has_digit) {
			        PREV(c);
			        if (min) PREV(min);
			        break;
			    }

			    /* bin_exp must reflect the FINAL lv (integer + fractional
			     * digits absorbed), not the integer part only. */
			    bin_exp = __LDL_bin_exp(&lv);

			    __LDL_normalize(&lv);          /* lv.l.exp = 16383 + b */
			    lv.l.exp += bin_exp;

			    /* Exponent part.  Accumulated separately and applied to
			     * dec_exp ONCE, so the decimal-point shift already in
			     * dec_exp is not multiplied by 10 per exponent digit. */
			    if (VAL(TOLOWER(c) == CX_e)) {
			        int d;
			        int e_sign = 1;
			        int has_ex_digit = 0;
			        long ex = 0;

			        NEXT(d);
			        if (VAL(d == CX_minus || d == CX_plus)) {
			            e_sign = (d == CX_minus) ? -1 : 1;
			            NEXT(d);
			        }

			        while (VAL(ISDIGIT(d))) {
			            ex = ex * 10 + (d - CX_0);
			            if (ex > 16383)
			                ex = 16383;     /* clamp magnitude only, before sign */
			            has_ex_digit = 1;
			            NEXT(d);
			        }

			        PREV(d);
			        if (!has_ex_digit) {
			            /* "10e" or "10e+": matching failure. */
			            has_digit = 0;
			            size = 0;
			        } else {
			            dec_exp += (int)(e_sign * ex);
			        }
			    } else {
			        PREV(c);
			    }

			    /* Saturate / underflow checks on the total decimal exponent. */
			    if (is_L) {
			        if (dec_exp > 4932)
			            special = 1;
			        else if (dec_exp < -4932 - 60)
			            lv.l.exp = lv.l.high = lv.l.middle = lv.l.low = 0;
			    } else {
			        if (dec_exp > 308)
			            special = 1;
			        else if (dec_exp < -308 - 44)
			            lv.l.exp = lv.l.high = lv.l.middle = lv.l.low = 0;
			    }

			    if (!special && dec_exp && lv.l.high)
			        lv = pow10_ld(lv, dec_exp); /* apply decimal scale */

			float_done:
			    if (!ignore && has_digit) {
			        if (special == 1) {
			            if (is_L) {
			                long double ld = (long double)INFINITY;
			                if (sign < 0)
			                    ld = -ld;
			                *va_arg(args, long double *) = ld;
			            } else {
			                double d = (double)INFINITY;
			                if (sign < 0)
			                    d = -d;
			                *va_arg(args, double *) = d;
			            }
			        } else if (special == 2) {
			            if (is_L)
			                *va_arg(args, long double *) = (long double)NAN;
			            else
			                *va_arg(args, double *) = (double)NAN;
			        } else if (special == 3) {
			            if (is_L) {
			                long double ld = sign < 0 ? -0.0L : 0.0L;
			                *va_arg(args, long double *) = ld;
			            } else {
			                double d = sign < 0 ? -0.0 : 0.0;
			                *va_arg(args, double *) = d;
			            }
			        } else {
			            if (is_L) {
			                long double ld = __ldl_to_long_double(&lv);
			                if (sign < 0)
			                    ld = -ld;
			                *va_arg(args, long double *) = ld;
			            } else {
			                double d;
			                /* a float is rounded once, not through double */
			                if (subtype != CX_l)
			                    __ldl_round_float(&lv);
			                d = __ldl_to_double(&lv);
			                if (sign < 0)
			                    d = -d;
			                *va_arg(args, double *) = d;
			            }
			        }
			        blocks++;
			    }
			    break;
			}
#endif
			case CX_percent:
				NEXT(c);
				if (c != CX_percent)
					PREV(c);
				break;
			case CX_n:
								if (!ignore)
				*va_arg(args, int *) = incount;
				size = 1;
				break;
			default: {
				unsigned long v = 0;
				int base;
				int min = 0;

				if (!type)
					ptr--;

				if (type == CX_p) {
					subtype = CX_l;
					type = CX_x;
				}

				if (VAL((c == CX_minus && type != CX_u) || c == CX_plus)) {
					min = c;
					NEXT(c);
				}

				/* %i takes its base from the prefix, and %x skips a 0x.
				 * Only one character is ever pushed back, which is all
				 * that ungetwc() keeps. */
				if ((type == CX_i || type == CX_x) && VAL(c == CX_0)) {
					int d;
					NEXT(d);
					if (VAL(TOLOWER(d) == CX_x)) {
						int e;
						NEXT(e);
						if (!VAL(ISXDIGIT(e))) {
							/* "0x" and no hex digit: a matching failure */
							PREV(e);
							size = 0;
							break;
						}
						type = CX_x;
						c = e;
					} else {
						if (type == CX_i)
							type = CX_o;
						PREV(d);
					}
				}

				base = (type == CX_x || type == CX_X) ? 16 : (type == CX_o ? 8 : 10);
				while (VAL(ISXDIGIT(c) && (base != 10 || ISDIGIT(c)) && (base != 8 || c <= CX_7))) {
					v = v * base + hexval(c);
					NEXT(c);
				}

				if (min && size == 2) {
					PREV(c);
					c = min;
				}

				PREV(c);

				if (ignore || !size)
					break;

				if (type == CX_u)
					switch (subtype) {
					case CX_l:
						case CX_L:
												*va_arg(args, unsigned long *) = v;
						break;
					case CX_i:
												*va_arg(args, unsigned int *) = v;
						break;
					case CX_h:
												*va_arg(args, unsigned short *) = v;
						break;
					}
				else {
					signed long v2;
					if (min == CX_minus)
						v2 = -v;
					else
						v2 = v;
					switch (subtype) {
					case CX_l:
						case CX_L:
												*va_arg(args, signed long *) = v2;
						break;
					case CX_i:
												*va_arg(args, signed int *) = v2;
						break;
					case CX_h:
												*va_arg(args, signed short *) = v2;
						break;
					}
				}
				blocks++;
				break;
			}
			}
			if (size && type != 'n' && type != '%')
				converted = 1;
			format = ptr;
		} else {
			if (ISSPACE((int )*format)) {
				do
					NEXT(c); while (ISSPACE(c));
				PREV(c);
				size = 1;
			} else {
				NEXT(c);
				if (c != *format)
					PREV(c);
			}
			format++;
		}
		if (!size)
			break;
	}
	__STDIO_UNLOCK(stream);
	if (c == EOF && !blocks && !converted)
		return c;
	else
		return blocks;
}
#endif // __SCANF_IMPL_H
