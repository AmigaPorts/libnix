#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>
#include <math.h>
#include "stdio.h"
#include "ami_printf_math.h"


/* some macros to cut this short
 * NEXT(c);     read next character
 * PREV(c);     ungetc a character
 * VAL(a)       leads to 1 if a is true and valid
 */

#define NEXT(c) do { \
    (c) = getc(stream); \
    if ((c) != EOF) { \
        size++; \
        incount++; \
    } \
} while(0)

#define PREV(c) do { \
    if ((c) != EOF) { \
        ungetc((c), stream); \
        size--; \
        incount--; \
    } \
} while(0)
#define VAL(a)  ((a)&&size<=width)

static inline int hexval(int c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}

#ifdef FULL_SPECIFIERS
static const unsigned char undef[3][sizeof(double)] = /* Undefined numeric values, IEEE */
{ { 0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* +inf */
{ 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* -inf */
{ 0x7f, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } /*  NaN */
};
#endif

#ifdef FULL_SPECIFIERS
extern unsigned char * __decimalpoint;
#endif

int vfscanf(FILE *stream, const char *format, va_list args) {
	__STDIO_LOCK(stream);
	size_t blocks = 0, incount = 0;
	int c = 0;
	/* EOF only for an input failure before the first conversion, even one
	   that assigns nothing (%*d) */
	int converted = 0;

	while (*format) {
		size_t size = 0;

		if (*format == '%') {
			size_t width = ULONG_MAX;
			char type, subtype = 'i', ignore = 0;
			const char *ptr = format + 1;
			size_t i;

			if (isdigit(*ptr)) {
				width = 0;
				while (isdigit(*ptr))
					width = width * 10 + (*ptr++ - '0');
			}

			while (*ptr == 'h' || *ptr == 'l' || *ptr == 'L' || *ptr == '*') {
				if (*ptr == '*')
					ignore = 1;
				else
					subtype = *ptr;
				ptr++;
			}

			type = *ptr++;

			if (type && type != '%' && type != 'c' && type != 'n'
					&& type != '[') {
				do /* ignore leading whitespace characters */
					NEXT(c); while (isspace(c));
				if (c == EOF)
					break;
				size = 1;
			} /* The first non-whitespace character is already read */

			switch (type) {
			case 'c': {
				unsigned char *bp;

				if (width == ULONG_MAX) /* Default */
					width = 1;

				if (!ignore)
					bp = va_arg(args, unsigned char *);
				else
					bp = NULL; /* Just to get the compiler happy */

				NEXT(c); /* 'c' did not skip whitespace */
				while (VAL(c!=EOF)) {
					if (!ignore)
						*bp++ = c;
					NEXT(c);
				}
				PREV(c);

				if (!ignore && size == width)
					blocks++;
				break;
			}
			case '[': {
				unsigned char *bp;
				unsigned char tab[32], a, b;
				char circflag = 0;

				if (*ptr == '^') {
					circflag = 1;
					ptr++;
				}
				for (i = 0; i < sizeof(tab); i++)
					tab[i] = circflag ? 255 : 0;
				for (;;) {
					if (!*ptr)
						break;
					a = b = *ptr++;
					if (*ptr == '-' && ptr[1] && ptr[1] != ']') {
						ptr++;
						b = *ptr++;
					}
					for (i = a; i <= b; i++)
						if (circflag)
							tab[i / 8] &= ~(1 << (i & 7));
						else
							tab[i / 8] |= 1 << (i & 7);
					if (*ptr == ']') {
						ptr++;
						break;
					}
				}

				if (!ignore)
					bp = va_arg(args, unsigned char *);
				else
					bp = NULL; /* Just to get the compiler happy */

				NEXT(c);
				while (VAL(c!=EOF && (tab[c/8] & (1<<(c&7))))) {
					if (!ignore)
						*bp++ = c;
					NEXT(c);
				}
				PREV(c);

				if (!ignore && size) {
					*bp++ = '\0';
					blocks++;
				}
				break;
			}
			case 's': {
				unsigned char *bp;

				if (!ignore)
					bp = va_arg(args, unsigned char *);
				else
					bp = NULL; /* Just to get the compiler happy */

				while (VAL(c!=EOF&&!isspace(c))) {
					if (!ignore)
						*bp++ = c;
					NEXT(c);
				}
				PREV(c);

				if (!ignore && size) {
					*bp++ = '\0';
					blocks++;
				}
				break;
			}
#ifdef FULL_SPECIFIERS
	        case 'e':
	        case 'f':
	        case 'g': {
	            LDL lv = { .l = { 0, 0, 0, 0 } };
	            int frac_digits = 0;
	            int min = 0, mine = 0;
	            int sign = 1;
	            int has_digit = 0;
	            int is_L = (subtype == 'L');
	            int special = 0;   /* 0 = finite, 1 = inf, 2 = nan, 3 = +/-0.0 */

	            if (VAL(c == '-' || c == '+')) {
	                min = c;
	                sign = (c == '-') ? -1 : 1;
	                NEXT(c);
	            }

	            /* inf / nan */
	            if (VAL(tolower(c) == 'i')) {
	                int d;
	                NEXT(d);
	                if (VAL(tolower(d) == 'n')) {
	                    int e;
	                    NEXT(e);
	                    if (VAL(tolower(e) == 'f')) {
	                        special = 1;
	                        has_digit = 1;
	                        goto float_done;
	                    }
	                    PREV(e);
	                }
	                PREV(d);
	            } else if (VAL(toupper(c) == 'N')) {
	                int d;
	                NEXT(d);
	                if (VAL(tolower(d) == 'a')) {
	                    int e;
	                    NEXT(e);
	                    if (VAL(toupper(e) == 'N')) {
	                        special = 2;
	                        has_digit = 1;
	                        goto float_done;
	                    }
	                    PREV(e);
	                }
	                PREV(d);
	            }

	            /* Integer part */
	            while (VAL(isdigit(c))) {
	                __LDL_mul_10_add(&lv, c - '0');
	                has_digit = 1;
	                NEXT(c);
	            }

	            if (c && VAL(c == 'x') && lv.l.high == 0 && lv.l.middle == 0 && lv.l.low == 0) {
	                size_t before_hex = size;   /* remember where 'x' started */
	                int has_hex = 0;
	                int bin_exp = 0;

	                NEXT(c);   /* consume 'x' */

	                while (VAL(isxdigit(c))) {
	                    __LDL_mul_16_add(&lv, hexval(c));
	                    has_hex = 1;
	                    NEXT(c);
	                }
	                if (VAL(c == __decimalpoint[0])) {
	                    NEXT(c);
	                    while (VAL(isxdigit(c))) {
	                        __LDL_mul_16_add(&lv, hexval(c));
	                        bin_exp -= 4;
	                        has_hex = 1;
	                        NEXT(c);
	                    }
	                }

	                if (!has_hex) {
	                    /* "0x" with no hex digits: scanf pushes back one character
	                     * only, so "0x" stays consumed and is a matching failure. */
	                    PREV(c);
	                    has_digit = 0;
	                    size = 0;
	                    goto float_done;
	                }

	                __LDL_normalize(&lv);

	                if (VAL(tolower(c) == 'p')) {
	                    int d;
	                    int pexp = 0;
	                    int psign_char = 0;   /* 0, '+' or '-' */
	                    int has_pexp = 0;

	                    NEXT(d);
	                    if (VAL(d == '-' || d == '+')) {
	                        psign_char = d;
	                        NEXT(d);
	                    }
	                    while (VAL(isdigit(d))) {
	                        pexp = pexp * 10 + (d - '0');
	                        has_pexp = 1;
	                        NEXT(d);
	                    }
	                    PREV(d);
	                    if (!has_pexp) {
	                        /* "0x1p" or "0x1p+": as for "0x", a matching failure */
	                        has_digit = 0;
	                        size = 0;
	                    } else {
	                        bin_exp += (psign_char == '-') ? -pexp : pexp;
	                    }
	                } else {
	                    PREV(c);
	                }
	                lv.l.exp += bin_exp;
	                goto float_done;
	            }

	            /* Fractional part */
	            if (VAL(c == __decimalpoint[0])) {
	                NEXT(c);
	                while (VAL(isdigit(c))) {
	                    __LDL_mul_10_add(&lv, c - '0');
	                    has_digit = 1;
	                    ++frac_digits;
	                    NEXT(c);
	                }
	            }

	            if (!has_digit) {
	                PREV(c);
	                if (min) PREV(min);
	                break;
	            }

	            __LDL_normalize(&lv);

	            if (frac_digits > 0)
	                lv = pow10_ld(lv, -frac_digits);

	            /* Exponent part */
	            if (VAL(tolower(c) == 'e')) {
	                int d;
	                int ex = 0;
	                int has_ex_digit = 0;

	                NEXT(d);
	                if (VAL(d == '-' || d == '+')) {
	                    mine = d;
	                    NEXT(d);
	                }

	                while (VAL(isdigit(d))) {
	                    ex = ex * 10 + (d - '0');
	                    has_ex_digit = 1;
	                    NEXT(d);
	                }

	                PREV(d);
	                if (!has_ex_digit) {
	                    /* "10e" or "10e+": as for "0x", a matching failure */
	                    has_digit = 0;
	                    size = 0;
	                } else {
                    	long new_exp = (long long)ex * 217706 / 65536;   /* ex * log2(10) in Q16 */
	                    if (mine == '-') {
                    		if (lv.l.exp - new_exp < 1L) {
                    			// underflow
                    			special = 3;
                    			goto float_done;
                    		}
							ex = -ex;
	                    } else if (lv.l.exp + new_exp > 32766) {
	                    	// overflow
							special = 1;
							goto float_done;
	                    }
	                    lv = pow10_ld(lv, ex);
	                }
	            } else {
	                PREV(c);
	            }

	        float_done:
	            if (!ignore && has_digit) {
	                if (special == 1) {
	                    /* inf */
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
	                    /* nan: sign of nan is not meaningful */
	                    if (is_L)
	                        *va_arg(args, long double *) = (long double)NAN;
	                    else
	                        *va_arg(args, double *) = (double)NAN;
	                } else if (special == 3) {
	                    /* underflow: result is +/-0.0 */
	                    if (is_L) {
	                        long double ld = sign < 0 ? -0.0L : 0.0L;
	                        *va_arg(args, long double *) = ld;
	                    } else {
	                        double d = sign < 0 ? -0.0 : 0.0;
	                        *va_arg(args, double *) = d;
	                    }
	                }
	                else {
	                    /* finite */
	                    if (is_L) {
	                        long double ld = __ldl_to_long_double(&lv);
	                        if (sign < 0)
	                            ld = -ld;
	                        *va_arg(args, long double *) = ld;
	                    } else {
	                        double d = __ldl_to_double(&lv);
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
			case '%':
				NEXT(c);
				if (c != '%')
					PREV(c); /* unget non-'%' character */
				break;
			case 'n':
				if (!ignore)
					*va_arg(args, int *) = incount;
				size = 1; /* fake a valid argument */
				break;
			default: {
				unsigned long v = 0;
				int base;
				int min = 0;

				if (!type)
					ptr--; /* unparse NUL character */

				if (type == 'p') {
					subtype = 'l'; /* This is the same as %lx */
					type = 'x';
				}

				if (VAL((c == '-' && type != 'u') || c == '+')) {
					min = c;
					NEXT(c);
				}

				if (type == 'i') /* which one to use ? */
				{
					if (VAL(c == '0')) /* Could be octal or sedecimal */
					{
						int d;
						NEXT(d); /* Get a look at next character */
						if (VAL(tolower(d)=='x')) {
							int e;
							NEXT(e); /* And the next */
							if (size>=width || VAL(isxdigit(c)))
								type = 'x'; /* Is a valid x number with '0x?' */
							PREV(e);
						} else
							type = 'o';
						PREV(d);
					} else if (VAL(!isdigit(c)&&isxdigit(c)))
						type = 'x'; /* Is a valid x number without '0x' */
				}

				if (type == 'x' && VAL(c == '0')) /* sedecimal */
				{
					int d;
					NEXT(d);
					if (VAL(tolower(d)=='x')) {
						int e;
						NEXT(e);
						if (VAL(isxdigit(e))) {
							c = e;
						} else {
							/* "0x" without digits: matching failure */
							PREV(e);
							size = 0;
							break;
						}
					} else
						PREV(d);
				}

				base = type == 'x' || type == 'X' ? 16 : (type == 'o' ? 8 : 10);
				while (VAL(isxdigit(c) && (base != 10 || isdigit(c)) && (base != 8 || c <= '7'))) {
				    v = v * base + hexval(c);
				    NEXT(c);
				}

				if (min && size == 2) /* If there is no valid character after sign, unget last */
				{
					PREV(c);
					c = min;
				}

				PREV(c);

				if (ignore || !size)
					break;

				if (type == 'u')
					switch (subtype) {
					case 'l':
					case 'L':
						*va_arg(args, unsigned long *) = v;
						break;
					case 'i':
						*va_arg(args, unsigned int *) = v;
						break;
					case 'h':
						*va_arg(args, unsigned short *) = v;
						break;
					}
				else {
					signed long v2;
					if (min == '-')
						v2 = -v;
					else
						v2 = v;
					switch (subtype) {
					case 'l':
					case 'L':
						*va_arg(args, signed long *) = v2;
						break;
					case 'i':
						*va_arg(args, signed int *) = v2;
						break;
					case 'h':
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
			if (isspace((int)*format)) {
				do
					NEXT(c); while (isspace(c));
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
