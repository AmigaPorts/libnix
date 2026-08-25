#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <math.h>
#include <float.h>
#include "stdio.h"

// a union to handle the double bits
union _d_bits {
    double d;
    struct {
        unsigned sign :1;
        unsigned exp :11;
        unsigned frac0 :20;
        unsigned frac1 :32;
    } b;
    unsigned u;
};

#ifdef FULL_SPECIFIERS
extern unsigned char *__decimalpoint;
#endif

extern int __vfprintf_total_size(FILE *stream, const char *fmt, va_list args);

/* a little macro to make life easier */
#define OUT(c)  do { \
    if (putc((c), stream) == EOF) { \
        __STDIO_UNLOCK(stream); \
        return -1; \
    } \
    outcount++; \
} while(0)

#define MINFLOATSIZE (DBL_DIG+3)
#define MININTSIZE (sizeof(unsigned long long)*CHAR_BIT/3+1)
#define MINPOINTSIZE (sizeof(void *)*CHAR_BIT/4+1)
#define REQUIREDBUFFER (MININTSIZE>MINPOINTSIZE? \
                        (MININTSIZE>MINFLOATSIZE?MININTSIZE:MINFLOATSIZE): \
                        (MINPOINTSIZE>MINFLOATSIZE?MINPOINTSIZE:MINFLOATSIZE))

/**
 * '#'
 * Used with o, exponent or X specifiers the value is preceeded with 0, 0x or 0X
 * respectively for values different than zero.
 * Used with a, A, e, E, f, F, g or G it forces the written output
 * to contain a decimal point even if no more digits follow.
 * By default, if no digits follow, no decimal point is written.
 */
#define ALTERNATEFLAG 1  /* '#' is set */

/**
 * '0'
 * Left-pads the number with zeroes (0) instead of spaces when padding is specified
 * (see width sub-specifier).
 */
#define ZEROPADFLAG   2  /* '0' is set */

/**
 * '-'
 * Left-justify within the given field width;
 * Right justification is the default (see width sub-specifier).
 */
#define LALIGNFLAG    4  /* '-' is set */

/**
 * ' '
 * If no sign is going to be written, a blank space is inserted before the value.
 */
#define BLANKFLAG     8  /* ' ' is set */

/**
 * '+'
 * Forces to preceed the result with a plus or minus sign (+ or -) even for positive numbers.
 * By default, only negative numbers are preceded with a - sign.
 */
#define SIGNFLAG      16 /* '+' is set */

static const char flagc[] = { '#', '0', '-', ' ', '+' };

/**
 * Set if an explicit precision is given.
 */
#define HAS_PRECI 32

extern unsigned __ulldivus(unsigned long long * llp, unsigned short n);

/**
 *  Differs from vfprintf such that it returns total number of bytes that
 *  would've been written if there were sufficient space in file.
 *  Required for vsnprintf
 */
int __vfprintf_total_size(FILE *stream, const char *format, va_list args) {
    size_t outcount = 0;

    __STDIO_LOCK(stream);

    while (*format) {
        if (*format == '%') {
            static const char lowertabel[] = { '0', '1', '2', '3', '4', '5',
                    '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
            static const char uppertabel[] = { '0', '1', '2', '3', '4', '5',
                    '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
            short width = 0;
            unsigned short preci = 0x7fff;
            short flags = 0; /* Specifications */
            char type, subtype = 'i';
            char buffer1[4]; /* Signs, prefixes (increased for safety) */
            char buffer[REQUIREDBUFFER + 8]; /* The body with extra safety margin */
            char *buffer2 = buffer; /* So we can set this to any other strings */
            size_t size1 = 0, size2 = 0; /* How many chars in buffer? */
            const char *ptr = format + 1; /* pointer to format string */
            unsigned short i, pad; /* Some temporary variables */

            do /* read flags */
                for (i = 0; i < sizeof(flagc); i++)
                    if (flagc[i] == *ptr) {
                        flags |= 1 << i;
                        ptr++;
                        break;
                    } while (i < sizeof(flagc));

            if (*ptr == '*') /* read width from arguments */
            {
                signed int a;
                ptr++;
                a = va_arg(args, signed int);
                if (a < 0) {
                    flags |= LALIGNFLAG;
                    width = -a;
                } else
                    width = a;
            } else {
                while (isdigit(*ptr))
                    width = width * 10 + (*ptr++ - '0');
            }

            if (*ptr == '.') {
                flags |= HAS_PRECI;
                ptr++;
                if (*ptr == '*') /* read precision from arguments */
                {
                    signed int a;
                    ptr++;
                    a = va_arg(args, signed int);
                    if (a >= 0)
                        preci = a;
                } else {
                    preci = 0;
                    while (isdigit(*ptr))
                        preci = preci * 10 + (*ptr++ - '0');
                }
            }

            if (*ptr == 'h' || *ptr == 'l' || *ptr == 'L' || *ptr == 'j'
                    || *ptr == 'z' || *ptr == 't') {
                subtype = *ptr++;
                if (*ptr == 'h' || *ptr == 'l')
                    ++ptr, ++subtype;
            } else
                subtype = 0;

            type = *ptr++;

            switch (type) {
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

                if (type == 'p') {
                    subtype = 'l'; /* This is written as %#lx */
                    type = 'x';
                    flags |= ALTERNATEFLAG;
                }

                if (type == 'd' || type == 'i') /* These are signed */
                {
                    signed long long v2;

                    /* Extract the argument with proper type */
                    if (subtype == 'l')
                        v2 = va_arg(args, signed long);
                    else if (subtype == 'm' || subtype == 'j')
                        v2 = va_arg(args, signed long long);
                    else
                        v2 = va_arg(args, signed int);

                    /*
                     * Apply length modifiers correctly for signed types.
                     * This is the critical fix for %hhd and %hd:
                     * - Use proper signed casts to preserve negative values
                     * - Don't use bitmasks which treat values as unsigned
                     */
                    if (subtype == 'h')
                        v2 = (short)v2;        /* Fix: signed short, not bitmask */
                    else if (subtype == 'i')
                        v2 = (signed char)v2;  /* Fix: signed char, not bitmask */

                    /* Now v2 is correctly sign-extended */
                    if (v2 < 0) {
                        if (size1 < sizeof(buffer1))
                            buffer1[size1++] = '-';
                        /*
                         * Handle LLONG_MIN safely - negating it would overflow.
                         * LLONG_MIN = -9223372036854775808
                         * LLONG_MAX + 1 = 9223372036854775808 (fits in unsigned long long)
                         */
                        if (v2 == LLONG_MIN) {
                            v = (unsigned long long)LLONG_MAX + 1ULL;
                        } else {
                            v = -v2;
                        }
                    } else {
                        if (flags & SIGNFLAG)
                            buffer1[size1++] = '+';
                        else if (flags & BLANKFLAG)
                            buffer1[size1++] = ' ';
                        v = v2;
                    }
                } else /* These are unsigned */
                {
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
                             * - If precision is specified and value is 0, don't add prefix
                             * - Otherwise add '0' prefix (including for 0 with default precision)
                             */
                            if (preci == 0x7fff || v) {
                                if (size1 < sizeof(buffer1))
                                    buffer1[size1++] = '0';
                            }
                        } else if ((type == 'x' || type == 'X') && v) {
                            if (size1 + 2 <= sizeof(buffer1)) {
                                buffer1[size1++] = '0';
                                buffer1[size1++] = type;
                            }
                        }
                    }
                }

                buffer2 = &buffer[sizeof(buffer)]; /* Calculate body string */
                base = type == 'x' || type == 'X' ? 16 : (type == 'o' ? 8 : 10);
                tabel = type != 'X' ? lowertabel : uppertabel;
                do {
                    if (buffer2 <= buffer) {
                        /* Should never happen with REQUIREDBUFFER, but safety check */
                        break;
                    }
                    *--buffer2 = tabel[__ulldivus(&v, base)];
                    size2++;
                } while (v);

                if (preci == 0x7fff) /* default */
                    preci = 0;
                else
                    flags &= ~ZEROPADFLAG;
                break;
            }
            case 'c': {
                char ch;
                if (subtype == 'l')
                    ch = (char)va_arg(args, long);
                else
                    ch = (char)va_arg(args, int);
                buffer[0] = ch;
                buffer2 = buffer;
                size2 = 1;
                preci = 0;
                break;
            }
            case 's': {
                buffer2 = va_arg(args, char *);
                if (buffer2 == NULL)
                    buffer2 = "(null)";
                size2 = strlen(buffer2);
                if (size2 > preci)
                    size2 = preci;
                preci = 0;
                break;
            }

#ifdef FULL_SPECIFIERS
            case 'a':
            case 'A':
            case 'f':
            case 'e':
            case 'E':
            case 'g':
            case 'G': {
                union _d_bits d;
                short exponent = 0;
                char sign = 0;
                const char * infnan = 0;
                char pad = (flags & ZEROPADFLAG) ? '0' : ' ';

                const int MEANINGFUL_DIGITS = DBL_DECIMAL_DIG;
                const int DIGITS_FOR_ROUNDING = MEANINGFUL_DIGITS + 1;

                if (type == 'f' || type == 'F')
                    type = 0;

                d = va_arg(args, union _d_bits);

                /* Fast sign check using bit operations */
                if (d.b.sign) {
                    d.b.sign = 0;
                    sign = '-';
                } else if (flags & SIGNFLAG)
                    sign = '+';
                else if (flags & BLANKFLAG)
                    sign = ' ';

                /* Fast inf/nan check using bit operations */
                if (d.b.exp == 0x7ff) {
                    if (d.b.frac0 || d.b.frac1) {
                        infnan = "NaN";
                        sign = 0;
                    } else {
                        infnan = "inf";
                    }
                    width -= 3;
                } else {
                    if (preci == 0x7fff) {
                        preci = 6;
                    }

                    /*
                     * OPTIMAL HYBRID APPROACH - NO frexp() CALLS!
                     *
                     * Step 1: Extract exponent directly from bits (super fast)
                     * Step 2: If extreme, use pow10 with integer exponent
                     * Step 3: If normal, use fast repeated multiply/divide
                     */
                    if (d.u || d.b.frac1) {
                        int bit_exp;
                        int normalized_exp;

                        /*
                         * Extract the exponent directly from bits.
                         * This is faster than frexp() because it's just a shift and mask.
                         */
                        bit_exp = d.b.exp;  /* Already in the union! */

                        if (bit_exp == 0) {
                            /* Subnormal number - exponent is -1022 */
                            bit_exp = -1022;
                        } else {
                            /* Normal number - subtract bias (1023) */
                            bit_exp -= 1023;
                        }

                        /*
                         * Approximate decimal exponent from binary exponent.
                         * log10(2) ~= 0.3010299956639811952
                         *
                         * Use integer arithmetic for speed:
                         * exponent = floor(bit_exp * 0.3010299956639811952)
                         */
                        if (bit_exp >= 0) {
                            normalized_exp = (int)((unsigned long long)bit_exp * 3010299956639811952LL / 10000000000000000000ULL);
                        } else {
                            normalized_exp = -((int)((unsigned long long)(-bit_exp) * 3010299956639811952LL / 10000000000000000000ULL));
                        }

                        /*
                         * Check if value is extreme (large exponent)
                         * For extreme values, use pow10 for accuracy
                         * For normal values, use fast iteration
                         */
                        if (bit_exp > 40 || bit_exp < -40) {
                            /*
                             * Extreme value: use pow10 for accuracy.
                             * This is still faster than frexp() + pow().
                             */
                            double mantissa;

                            /* Get mantissa in [1, 10) using pow10 */
                            if (normalized_exp >= 0) {
                                mantissa = d.d / pow(10.0, normalized_exp);
                            } else {
                                mantissa = d.d * pow(10.0, -normalized_exp);
                            }

                            /* Fine-tune with minimal iterations (0-2) */
                            while (mantissa >= 10.0 && normalized_exp < 308) {
                                mantissa *= 0.1;
                                ++normalized_exp;
                            }
                            while (mantissa < 1.0 && normalized_exp > -308) {
                                mantissa *= 10.0;
                                --normalized_exp;
                            }

                            d.d = mantissa;
                            exponent = normalized_exp;
                        } else {
                            /*
                             * Normal value: use fast repeated multiply/divide.
                             * This is the fastest approach for normal values.
                             */
                            if (d.d >= 1) {
                                while (d.d >= 10) {
                                    d.d *= 0.1;
                                    ++normalized_exp;
                                }
                            } else {
                                while (d.d < 1) {
                                    d.d *= 10;
                                    --normalized_exp;
                                }
                            }
                            exponent = normalized_exp;
                        }
                    } else {
                        exponent = 0;
                    }
                }

                {
                    short pos;
                    unsigned x, y;

                    int startPos = 1;
                    short stopPos;
                    short leading = 1;
                    short dotZero = 0;
                    short postZero = 0;
                    short killZero = 0;
                    int extra_zeros = 0;
                    int original_preci = preci;
                    int computed_fractional = 0;

                    if (!infnan) {
                        if (type != 'a' && type != 'A') {

                            if (type == 'g' || type == 'G') {
                                int limit = preci + 4;
                                --preci;
                                if (preci >= exponent && exponent >= 0) {
                                    type = 0;
                                    preci -= exponent;
                                } else if (exponent < 0 && limit + exponent + 1 > preci) {
                                    type = 0;
                                    preci -= exponent;
                                } else {
                                    type = 'e';
                                }
                                stopPos = preci + 1;
                                if (!(flags & ALTERNATEFLAG))
                                    killZero = 1;
                            }

                            int digits_to_compute;

                            if (type == 0) {
                                int integer_digits = (exponent >= 0) ? (exponent + 1) : 1;
                                int meaningful_fractional = MEANINGFUL_DIGITS - integer_digits;
                                if (meaningful_fractional < 0)
                                    meaningful_fractional = 0;

                                int frac_to_compute = meaningful_fractional + 1;
                                if (frac_to_compute > preci + 1)
                                    frac_to_compute = preci + 1;

                                digits_to_compute = integer_digits + frac_to_compute;
                                stopPos = digits_to_compute;
                            } else {
                                int meaningful_fractional = MEANINGFUL_DIGITS - 1;
                                if (meaningful_fractional < 0)
                                    meaningful_fractional = 0;

                                int frac_to_compute = meaningful_fractional + 1;
                                if (frac_to_compute > preci + 1)
                                    frac_to_compute = preci + 1;

                                digits_to_compute = 1 + frac_to_compute;
                                stopPos = digits_to_compute;
                            }

                            buffer[0] = '0';
                            pos = 1;
                            if (type == 0) {
                                if (exponent >= 0) {
                                    leading += exponent;
                                    stopPos += exponent + 1;
                                } else {
                                    dotZero = -exponent - 1;
                                    buffer[1] = '0';
                                    if (dotZero > preci) {
                                        dotZero = preci;
                                        buffer[2] = '0';
                                        pos = 3;
                                    } else
                                        pos = 2;
                                    stopPos -= dotZero - 1;
                                }
                            } else {
                                ++stopPos;
                            }

                            if (stopPos >= (short)(sizeof(buffer) - 2)) {
                                postZero = stopPos - (sizeof(buffer) - 2) + 1;
                                if (type == 0 && postZero > preci)
                                    postZero = preci;
                                stopPos = sizeof(buffer) - 2;
                            }

                            for (; pos < stopPos; ++pos) {
                                int z = (int) d.d;
                                if (z) {
                                    d.d = (d.d - z) * 10;
                                    if (d.d <= -0.1) {
                                        --z;
                                        d.d += 10;
                                    }
                                } else
                                    d.d *= 10;
                                buffer[pos] = (char) ('0' + z);
                            }

                            if (d.d >= 5.) {
                                --pos;
                                for (; pos >= startPos; --pos) {
                                    if (++buffer[pos] <= '9')
                                        break;
                                    buffer[pos] = '0';
                                }
                                if (pos <= startPos) {
                                    if (type != 0) {
                                        if (pos < startPos) {
                                            startPos = pos;
                                            buffer[pos] = '1';
                                        }
                                        ++exponent;
                                        --stopPos;
                                    } else if (type == 0) {
                                        if (exponent < 0) {
                                            if (dotZero > 0) {
                                                startPos = 0;
                                                --dotZero;
                                            }
                                        } else if (pos < startPos ){
                                            startPos = pos;
                                            buffer[pos] = '1';
                                            ++leading;
                                        }
                                    }
                                }
                            }

                            preci = original_preci;

                            if (type == 0) {
                                int integer_digits = (exponent >= 0) ? (exponent + 1) : 1;
                                computed_fractional = (stopPos - startPos) - integer_digits;
                                if (computed_fractional < 0)
                                    computed_fractional = 0;

                                int needed_fractional = preci;
                                if (exponent < 0)
                                    needed_fractional += (-exponent);

                                extra_zeros = needed_fractional - computed_fractional;
                                if (extra_zeros < 0)
                                    extra_zeros = 0;
                            } else {
                                computed_fractional = (stopPos - startPos) - 1;
                                if (computed_fractional < 0)
                                    computed_fractional = 0;

                                extra_zeros = preci - computed_fractional;
                                if (extra_zeros < 0)
                                    extra_zeros = 0;
                            }

                        } else {
                            /* 'a' 'A' format - pure bit operations */
                            if (preci == 0x7fff)
                                preci = 0;
                            if (!(flags & HAS_PRECI)) killZero = 1;

                            OUT('0');
                            OUT(type + 'X' - 'A');

                            if (!d.b.exp) {
                                exponent = x = y = 0;
                            } else {
                                exponent = d.b.exp - 1023;
                                x = d.b.frac0;
                                y = d.b.frac1;
                                buffer[0] = '1';
                                startPos = 0;
                                if (HAS_PRECI)
                                    ++preci;
                            }
                            stopPos = 1;

                            int max_hex_digits = (sizeof(unsigned long long) * CHAR_BIT + 3) / 4;
                            int user_preci = preci;

                            int hex_to_compute = max_hex_digits + 1;
                            if (hex_to_compute > preci + 1)
                                hex_to_compute = preci + 1;

                            {unsigned j; for (j = 16;j <= 28; j += 12) {
                                {int i; for (i = j; i >= 0; i -= 4) {
                                    unsigned c = (x >> i) & 0xf;
                                    x -= c << i;
                                    if (c > 9)
                                        c += type - 10;
                                    else
                                        c += '0';
                                    if (stopPos < (short)sizeof(buffer))
                                        buffer[stopPos++] = c;
                                    if (stopPos >= hex_to_compute + 1 || (!x && !y && stopPos >= preci))
                                        break;
                                }}
                                if (stopPos >= hex_to_compute + 1 || (!y && stopPos >= preci))
                                    break;
                                x = y;
                                y = 0;
                            }}

                            int computed_hex_digits = stopPos - startPos;
                            if (computed_hex_digits < 0) computed_hex_digits = 0;

                            preci = user_preci;
                            extra_zeros = preci - computed_hex_digits;
                            if (extra_zeros < 0) extra_zeros = 0;

                            type += 'P' - 'A';
                        }

                        if (killZero != 0) {
                            int stop = stopPos - 1;
                            while (stop > startPos) {
                                if (buffer[stop] != '0')
                                    break;
                                if (type == 0 && stop - startPos == exponent)
                                    break;
                                --stop;
                            }
                            if (type != 0 && stop - startPos + 5 < width)
                                width = stop - startPos + preci + 5;
                            else if (type == 0) {
                                if (stop + 1 != stopPos
                                        && stop - startPos == exponent)
                                    ++width;
                            }
                            stopPos = stop + 1;
                        }

                        if (type != 0) {
                            width -= 5 + stopPos - startPos + postZero;

                            if (exponent < -99) {
                                --width;
                                if (exponent < -999)
                                    --width;
                            } else if (exponent > 99) {
                                --width;
                                if (exponent > 999)
                                    --width;
                            }
                        } else {
                            if (leading > stopPos - startPos)
                                width -= leading + dotZero + postZero;
                            else
                                width -= stopPos - startPos + dotZero + postZero;
                        }

                        if (preci > 0 || (flags & ALTERNATEFLAG) != 0)
                            --width;
                    }
                    if (sign != 0)
                        --width;

                    if ((flags & LALIGNFLAG) == 0)
                        while (--width >= 0)
                            OUT(pad);

                    if (sign != 0)
                        OUT(sign);

                    if (infnan) {
                        OUT(infnan[0]);
                        OUT(infnan[1]);
                        OUT(infnan[2]);
                    } else {
                        int fractional_output = 0;

                        while (leading-- > 0 && startPos < stopPos)
                            OUT(buffer[startPos++]);

                        while (leading-- >= 0)
                            OUT('0');

                        if (startPos < stopPos || dotZero != 0 || postZero != 0
                                || (flags & ALTERNATEFLAG) != 0) {
                            if (__decimalpoint && __decimalpoint[0])
                                OUT(__decimalpoint[0]);
                            else
                                OUT('.');
                        }

                        while (dotZero-- > 0) {
                            OUT('0');
                            fractional_output++;
                        }

                        for (; startPos < stopPos; ++startPos) {
                            OUT(buffer[startPos]);
                            fractional_output++;
                        }

                        if (type != 0) {
                            int zeros_to_add = preci - computed_fractional;
                            if (zeros_to_add < 0) zeros_to_add = 0;
                            while (zeros_to_add-- > 0) {
                                OUT('0');
                                fractional_output++;
                            }
                        } else {
                            int needed_fractional = preci;
                            if (exponent < 0)
                                needed_fractional += (-exponent);

                            int zeros_to_add = needed_fractional - fractional_output;
                            if (zeros_to_add < 0) zeros_to_add = 0;
                            while (zeros_to_add-- > 0) {
                                OUT('0');
                                fractional_output++;
                            }
                        }

                        while (postZero-- > 0)
                            OUT('0');

                        if (type != 0) {
                            int xout;

                            OUT(type);
                            if (exponent < 0) {
                                OUT('-');
                                exponent = -exponent;
                            } else
                                OUT('+');
                            --width;

                            xout = 0;
                            if (exponent > 999) {
                                int z = exponent / 1000;
                                OUT('0' + z);
                                exponent -= z * 1000;
                                --width;
                                xout = 1;
                            }
                            if (xout || exponent > 99) {
                                int z = exponent / 100;
                                OUT('0' + z);
                                exponent -= z * 100;
                                --width;
                                xout = 1;
                            }
                            if (xout || exponent > 9 || (type | 0x20) != 'p' ) {
                                int z = exponent / 10;
                                OUT('0' + z);
                                exponent -= z * 10;
                                --width;
                            }
                            OUT('0' + exponent);
                            --width;
                        }
                    }

                    if ((flags & LALIGNFLAG) != 0)
                        while (--width >= 0)
                            OUT(' ');
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
                *va_arg(args, int *) = outcount;
                width = preci = 0;
                break;
            default:
                if (!type)
                    ptr--; /* We've gone too far - step one back */
                buffer2 = (char *) format;
                size2 = ptr - format;
                width = preci = 0;
                break;
            }

            if (flags & HAS_PRECI) {
                if (size2 == 1 && !preci && buffer2[0] == '0')
                    size2 = 0;
            }

            pad = size1 + (size2 >= preci ? size2 : preci); /* Calculate the number of characters */
            pad = pad >= width ? 0 : width - pad; /* and the number of resulting pad bytes */

            if (flags & ZEROPADFLAG) /* print sign and that like */
                for (i = 0; i < size1; i++)
                    OUT(buffer1[i]);

            if (!(flags & LALIGNFLAG)) /* Pad left */
                for (i = 0; i < pad; i++)
                    OUT(flags&ZEROPADFLAG?'0':' ');

            if (!(flags & ZEROPADFLAG)) /* print sign if not zero padded */
                for (i = 0; i < size1; i++)
                    OUT(buffer1[i]);

            for (i = size2; i < preci; i++) /* extend to precision */
                OUT('0');

            for (i = 0; i < size2; i++) /* print body */
                OUT(buffer2[i]);

            if (flags & LALIGNFLAG) /* Pad right */
                for (i = 0; i < pad; i++)
                    OUT(' ');

            format = ptr;
        } else
            OUT(*format++);
    }
    __STDIO_UNLOCK(stream);

    return outcount;
}

#ifdef TESTME
int main(int argc, char ** argv) {
	double d;
	printf("%-20.0f|\n", 0.0);
	printf("%-20.0e\n", 0.0);
	printf("%-20.1g\n", 0.0);
	printf("%#20.0f\n", 0.0);
	printf("%#20.0e\n", 0.0);
	printf("%#20.1g\n", 0.0);

	d = 1.2345678902468e-13;
	for (int i = 0; i < 24; ++i) {
		d *= 10;
		printf("%20.7f\n", d);
	}

	d = 1.2345678902468e-13;
	for (int i = 0; i < 24; ++i) {
		d *= 10;
		printf("%20.7g\n", d);
	}

	d = 1.2345678902468e-13;
	for (int i = 0; i < 24; ++i) {
		d *= 10;
		printf("%20.7e\n", d);
	}
#define PI 3.4145926535988
	d = 1.2345678902468e-13 * PI;
        double x =  3.23454989171 - PI;
	d = d / x;
	printf("%1.40e\n", d);

	return 0;
}
#endif
