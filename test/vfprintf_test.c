/* ==========================================================================
 *  vfprintf_test.c
 *
 *  Self-test harness for the narrow printf engine.
 *  Build with  -DFULL_SPECIFIERS and link against
 *  __vfprintf_total_size.o and ami_printf_math.o.
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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "ami_printf_math.h"

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
	d = 1.2345678902468e-13L;
	TESTFMT("f e-13|           0.0000000|", "%20.7f", d);
	d = 1.2345678902468e-12L;
	TESTFMT("f e-12|           0.0000000|", "%20.7f", d);
	d = 1.2345678902468e-11L;
	TESTFMT("f e-11|           0.0000000|", "%20.7f", d);
	d = 1.2345678902468e-10L;
	TESTFMT("f e-10|           0.0000000|", "%20.7f", d);
	d = 1.2345678902468e-09L;
	TESTFMT("f e-9 |           0.0000000|", "%20.7f", d);
	d = 1.2345678902468e-08L;
	TESTFMT("f e-8 |           0.0000000|", "%20.7f", d);
	d = 1.2345678902468e-07L;
	TESTFMT("f e-7 |           0.0000001|", "%20.7f", d);
	d = 1.2345678902468e-06L;
	TESTFMT("f e-6 |           0.0000012|", "%20.7f", d);
	d = 1.2345678902468e-05L;
	TESTFMT("f e-5 |           0.0000123|", "%20.7f", d);
	d = 1.2345678902468e-04L;
	TESTFMT("f e-4 |           0.0001235|", "%20.7f", d);
	d = 1.2345678902468e-03L;
	TESTFMT("f e-3 |           0.0012346|", "%20.7f", d);
	d = 1.2345678902468e-02L;
	TESTFMT("f e-2 |           0.0123457|", "%20.7f", d);
	d = 1.2345678902468e-01L;
	TESTFMT("f e-1 |           0.1234568|", "%20.7f", d);
	d = 1.2345678902468e+00L;
	TESTFMT("f e0  |           1.2345679|", "%20.7f", d);
	d = 1.2345678902468e+01L;
	TESTFMT("f e1  |          12.3456789|", "%20.7f", d);
	d = 1.2345678902468e+02L;
	TESTFMT("f e2  |         123.4567890|", "%20.7f", d);
	d = 1.2345678902468e+03L;
	TESTFMT("f e3  |        1234.5678902|", "%20.7f", d);
	d = 1.2345678902468e+04L;
	TESTFMT("f e4  |       12345.6789025|", "%20.7f", d);
	d = 1.2345678902468e+05L;
	TESTFMT("f e5  |      123456.7890247|", "%20.7f", d);
	d = 1.2345678902468e+06L;
	TESTFMT("f e6  |     1234567.8902468|", "%20.7f", d);
	d = 1.2345678902468e+07L;
	TESTFMT("f e7  |    12345678.9024680|", "%20.7f", d);
	d = 1.2345678902468e+08L;
	TESTFMT("f e8  |   123456789.0246800|", "%20.7f", d);
	d = 1.2345678902468e+09L;
	TESTFMT("f e9  |  1234567890.2467999|", "%20.7f", d); //!!
	d = 1.2345678902468e+10L;
	TESTFMT("f e10 | 12345678902.4680000|", "%20.7f", d);
	printf("\n");
	/* ==================================================================
	 * SECTION 4: General format (%g) decade walk
	 * ================================================================== */
	printf("--- General %%g decade walk ---\n");
	d = 1.2345678902468e-13L;
	TESTFMT("g e-13|        1.234568e-13|", "%20.7g", d); // value length = 12 -> 8 spaces
	d = 1.2345678902468e-12L;
	TESTFMT("g e-12|        1.234568e-12|", "%20.7g", d); // value length = 12 -> 8 spaces
	d = 1.2345678902468e-11L;
	TESTFMT("g e-11|        1.234568e-11|", "%20.7g", d); // value length = 12 -> 8 spaces
	d = 1.2345678902468e-10L;
	TESTFMT("g e-10|        1.234568e-10|", "%20.7g", d); // value length = 12 -> 8 spaces
	d = 1.2345678902468e-09L;
	TESTFMT("g e-9 |        1.234568e-09|", "%20.7g", d); // value length = 12 -> 8 spaces
	d = 1.2345678902468e-08L;
	TESTFMT("g e-8 |        1.234568e-08|", "%20.7g", d); // value length = 12 -> 8 spaces
	d = 1.2345678902468e-07L;
	TESTFMT("g e-7 |        1.234568e-07|", "%20.7g", d); // value length = 12 -> 8 spaces
	d = 1.2345678902468e-06L;
	TESTFMT("g e-6 |        1.234568e-06|", "%20.7g", d); // value length = 12 -> 8 spaces
	d = 1.2345678902468e-05L;
	TESTFMT("g e-5 |        1.234568e-05|", "%20.7g", d); // (value length = 12 -> 8 spaces)
	d = 1.2345678902468e-04L;
	TESTFMT("g e-4 |        0.0001234568|", "%20.7g", d); // (value length = 12 -> 8 spaces)
	d = 1.2345678902468e-03L;
	TESTFMT("g e-3 |         0.001234568|", "%20.7g", d); // (value length = 11 -> 9 spaces)
	d = 1.2345678902468e-02L;
	TESTFMT("g e-2 |          0.01234568|", "%20.7g", d); // (value length = 10 -> 10 spaces)
	d = 1.2345678902468e-01L;
	TESTFMT("g e-1 |           0.1234568|", "%20.7g", d); // (value length = 9 -> 11 spaces)
	d = 1.2345678902468e+00L;
	TESTFMT("g e0  |            1.234568|", "%20.7g", d); // (value length = 8 -> 12 spaces)
	d = 1.2345678902468e+01L;
	TESTFMT("g e1  |            12.34568|", "%20.7g", d); // (value length = 8 -> 12 spaces)
	d = 1.2345678902468e+02L;
	TESTFMT("g e2  |            123.4568|", "%20.7g", d); // (value length = 8 -> 12 spaces)
	d = 1.2345678902468e+03L;
	TESTFMT("g e3  |            1234.568|", "%20.7g", d); // (value length = 8 -> 12 spaces)
	d = 1.2345678902468e+04L;
	TESTFMT("g e4  |            12345.68|", "%20.7g", d); // (value length = 8 -> 12 spaces)
	d = 1.2345678902468e+05L;
	TESTFMT("g e5  |            123456.8|", "%20.7g", d); // (value length = 8 -> 12 spaces)
	d = 1.2345678902468e+06L;
	TESTFMT("g e6  |             1234568|", "%20.7g", d); // (value length = 7 -> 13 spaces)
	d = 1.2345678902468e+07L;
	TESTFMT("g e7  |        1.234568e+07|", "%20.7g", d); // value length = 12 -> 8 spaces
	d = 1.2345678902468e+08L;
	TESTFMT("g e8  |        1.234568e+08|", "%20.7g", d); // value length = 12 -> 8 spaces
	d = 1.2345678902468e+09L;
	TESTFMT("g e9  |        1.234568e+09|", "%20.7g", d); // value length = 12 -> 8 spaces
	d = 1.2345678902468e+10L;
	TESTFMT("g e10 |        1.234568e+10|", "%20.7g", d); // value length = 12 -> 8 spaces
	printf("\n");
	/* ==================================================================
	 * SECTION 5: Scientific (%e) decade walk
	 * ================================================================== */
	printf("--- Scientific %%e decade walk ---\n");
	d = 1.2345678902468e-13L;
	TESTFMT("e e-13|       1.2345679e-13|", "%20.7e", d);
	d = 1.2345678902468e-12L;
	TESTFMT("e e-12|       1.2345679e-12|", "%20.7e", d);
	d = 1.2345678902468e-11L;
	TESTFMT("e e-11|       1.2345679e-11|", "%20.7e", d);
	d = 1.2345678902468e-10L;
	TESTFMT("e e-10|       1.2345679e-10|", "%20.7e", d);
	d = 1.2345678902468e-09L;
	TESTFMT("e e-9 |       1.2345679e-09|", "%20.7e", d);
	d = 1.2345678902468e-08L;
	TESTFMT("e e-8 |       1.2345679e-08|", "%20.7e", d);
	d = 1.2345678902468e-07L;
	TESTFMT("e e-7 |       1.2345679e-07|", "%20.7e", d);
	d = 1.2345678902468e-06L;
	TESTFMT("e e-6 |       1.2345679e-06|", "%20.7e", d);
	d = 1.2345678902468e-05L;
	TESTFMT("e e-5 |       1.2345679e-05|", "%20.7e", d);
	d = 1.2345678902468e-04L;
	TESTFMT("e e-4 |       1.2345679e-04|", "%20.7e", d);
	d = 1.2345678902468e-03L;
	TESTFMT("e e-3 |       1.2345679e-03|", "%20.7e", d);
	d = 1.2345678902468e-02L;
	TESTFMT("e e-2 |       1.2345679e-02|", "%20.7e", d);
	d = 1.2345678902468e-01L;
	TESTFMT("e e-1 |       1.2345679e-01|", "%20.7e", d);
	d = 1.2345678902468e+00L;
	TESTFMT("e e0  |       1.2345679e+00|", "%20.7e", d);
	d = 1.2345678902468e+01L;
	TESTFMT("e e1  |       1.2345679e+01|", "%20.7e", d);
	d = 1.2345678902468e+02L;
	TESTFMT("e e2  |       1.2345679e+02|", "%20.7e", d);
	d = 1.2345678902468e+03L;
	TESTFMT("e e3  |       1.2345679e+03|", "%20.7e", d);
	d = 1.2345678902468e+04L;
	TESTFMT("e e4  |       1.2345679e+04|", "%20.7e", d);
	d = 1.2345678902468e+05L;
	TESTFMT("e e5  |       1.2345679e+05|", "%20.7e", d);
	d = 1.2345678902468e+06L;
	TESTFMT("e e6  |       1.2345679e+06|", "%20.7e", d);
	d = 1.2345678902468e+07L;
	TESTFMT("e e7  |       1.2345679e+07|", "%20.7e", d);
	d = 1.2345678902468e+08L;
	TESTFMT("e e8  |       1.2345679e+08|", "%20.7e", d);
	d = 1.2345678902468e+09L;
	TESTFMT("e e9  |       1.2345679e+09|", "%20.7e", d);
	d = 1.2345678902468e+10L;
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
		long double pi_ld = 3.141592653589793238L;
		TESTFMT("ld high prec|3.141592653589793238|", "%.18Lf", pi_ld);
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
	TESTFMT("f huge prec|1.2345678901234567|", "%.16f", 1.2345678901234567);
	TESTFMT("e huge prec|1.2345678901234567e+00|", "%.16e", 1.2345678901234567);
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
	 * SECTION 16: Long padding (streams via OUT, never buffered)
	 *
	 * These exercise the piece-list pad slot with widths far larger than
	 * outbuffer[FLOAT_OUTBUF_MIN].  Before the piece-list rework, any
	 * width above ~140 would have overrun outbuffer via TOUT('0')/TOUT(' ').
	 * ================================================================== */
	printf("--- Long padding tests ---\n");
	{
		char big[2048];
		int n;

		/* Left-align with a huge width: the tail pad is a single
		 * appended piece with startPos == outp. */
		memset(big, 0, sizeof(big));
		n = snprintf(big, sizeof(big), "%-1500.2f", 3.14);
		tests_run++;
		if (n == 1500 &&
				big[0] == '3' && big[1] == '.' && big[2] == '1' && big[3] == '4' &&
				big[1499] == ' ' && big[1500] == '\0') {
			tests_passed++;
			printf("OK: %%-1500.2f left-align (1500 chars)\n");
		} else {
			tests_failed++;
			printf("FAIL: %%-1500.2f left-align: n=%d\n", n);
		}

		/* Right-align, space pad: the front pad slot at index 0 holds
		 * 1496 spaces, then "3.14". */
		memset(big, 0, sizeof(big));
		n = snprintf(big, sizeof(big), "%1500.2f", 3.14);
		tests_run++;
		if (n == 1500 &&
				big[0] == ' ' && big[1495] == ' ' &&
				big[1496] == '3' && big[1497] == '.' &&
				big[1498] == '1' && big[1499] == '4' &&
				big[1500] == '\0') {
			tests_passed++;
			printf("OK: %%1500.2f right-align (1500 chars)\n");
		} else {
			tests_failed++;
			printf("FAIL: %%1500.2f right-align: n=%d\n", n);
		}

		/* Right-align, zero pad, with sign: the front slot is anchored
		 * at offset 1 (after the '-'), so the sign comes first, then
		 * 1495 zeros, then "3.14". */
		memset(big, 0, sizeof(big));
		n = snprintf(big, sizeof(big), "%01500.2f", -3.14);
		tests_run++;
		if (n == 1500 &&
				big[0] == '-' && big[1] == '0' && big[1495] == '0' &&
				big[1496] == '3' && big[1497] == '.' &&
				big[1498] == '1' && big[1499] == '4' &&
				big[1500] == '\0') {
			tests_passed++;
			printf("OK: %%01500.2f right-align zero-pad with sign (1500 chars)\n");
		} else {
			tests_failed++;
			printf("FAIL: %%01500.2f right-align zero-pad: n=%d\n", n);
		}

		/* ~100 leading fractional zeros: 1e-100 in %f.  The value
		 * after the point is 99 zeros then "1".  This exercises the
		 * fractional_zeros PUSH in S6 step 5; before the piece-list
		 * rework, that loop did TOUT('0') 99 times into outbuffer.
		 *
		 * Layout for "%.101f" of 1e-100 (n = 103):
		 *   [0]    '0'
		 *   [1]    '.'
		 *   [2..100]  99 '0'           <- fractional zeros
		 *   [101]  '1'                 <- first significant digit
		 *   [102]  '0'                 <- padded to preci = 101
		 *   [103]  '\0'
		 *
		 * Use 1.0e-100 which is exactly representable enough that the
		 * digit stream is [9x '0', '1', '0'] as derived above; if the
		 * platform's rounding at the 101st fractional digit differs
		 * by one, only the last digit (the padded '0') can change.
		 */
		memset(big, 0, sizeof(big));
		n = snprintf(big, sizeof(big), "%.101f", 1.0e-100);
		tests_run++;
		if (n == 103 &&
				big[0] == '0' && big[1] == '.' &&
				big[2] == '0' && big[100] == '0' &&
				big[101] == '1' &&
				big[102] == '0' &&
				big[103] == '\0') {
			tests_passed++;
			printf("OK: %%.101f of 1e-100 (99 leading fractional zeros)\n");
		} else {
			tests_failed++;
			printf("FAIL: %%.101f of 1e-100: n=%d\n", n);
			if (n > 0 && n < (int) sizeof(big)) {
				printf("      [0..8]       = '%.*s'\n", 9, big);
				printf("      [98..107]    = '%.*s'\n", 10, big + 98);
			}
		}

		/* ~115 pad zeros on a signed float: %0120.2f on -3.14.
		 * front slot anchored at offset 1 (after '-'), so the zeros
		 * sit between the sign and the magnitude.  This is the exact
		 * case the pieces[0].startPos == 1 branch exists for.
		 *
		 * Layout (n = 120):
		 *   [0]     '-'
		 *   [1..115]  115 '0'          <- pad
		 *   [116]   '3'
		 *   [117]   '.'
		 *   [118]   '1'
		 *   [119]   '4'
		 *   [120]   '\0'
		 */
		memset(big, 0, sizeof(big));
		n = snprintf(big, sizeof(big), "%0120.2f", -3.14);
		tests_run++;
		if (n == 120 &&
				big[0] == '-' &&
				big[1] == '0' && big[115] == '0' &&
				big[116] == '3' && big[117] == '.' &&
				big[118] == '1' && big[119] == '4' &&
				big[120] == '\0') {
			tests_passed++;
			printf("OK: %%0120.2f right-align zero-pad, 115 pad zeros\n");
		} else {
			tests_failed++;
			printf("FAIL: %%0120.2f: n=%d\n", n);
			if (n > 0 && n < (int) sizeof(big)) {
				printf("      [0..8]     = '%.*s'\n", 9, big);
				printf("      [113..121] = '%.*s'\n", 9, big + 113);
			}
		}

		/* Same pad count on the integer path, as a control: %0120d
		 * on -42 produces '-' + 117 zeros + "42".  The integer path
		 * has always used OUT for padding, so this must pass on both
		 * the old and new code; if it fails, the shared buffer[] or
		 * pad logic was disturbed by the float-path changes.
		 *
		 * Layout (n = 120):
		 *   [0]     '-'
		 *   [1..117]  117 '0'
		 *   [118]   '4'
		 *   [119]   '2'
		 *   [120]   '\0'
		 */
		memset(big, 0, sizeof(big));
		n = snprintf(big, sizeof(big), "%0120d", -42);
		tests_run++;
		if (n == 120 &&
				big[0] == '-' &&
				big[1] == '0' && big[117] == '0' &&
				big[118] == '4' && big[119] == '2' &&
				big[120] == '\0') {
			tests_passed++;
			printf("OK: %%0120d right-align zero-pad, 117 pad zeros\n");
		} else {
			tests_failed++;
			printf("FAIL: %%0120d: n=%d\n", n);
			if (n > 0 && n < (int) sizeof(big)) {
				printf("      [0..8]     = '%.*s'\n", 9, big);
				printf("      [115..123] = '%.*s'\n", 9, big + 115);
			}
		}

		/* Huge precision on %e: the trailing-zeros run is a single
		 * PUSH with count == preci - 1, so no digit buffer growth.
		 *
		 * Layout for "%.1200e" of 1.5 (n = 1206):
		 *   [0]    '1'
		 *   [1]    '.'
		 *   [2]    '5'
		 *   [3..1201]  1199 '0'
		 *   [1202] 'e'
		 *   [1203] '+'
		 *   [1204] '0'
		 *   [1205] '0'
		 *   [1206] '\0'
		 */
		memset(big, 0, sizeof(big));
		n = snprintf(big, sizeof(big), "%.1200e", 1.5);
		tests_run++;
		if (n == 1206 &&
				big[0] == '1' && big[1] == '.' && big[2] == '5' &&
				big[3] == '0' && big[1201] == '0' &&
				big[1202] == 'e' && big[1203] == '+' &&
				big[1204] == '0' && big[1205] == '0' &&
				big[1206] == '\0') {
			tests_passed++;
			printf("OK: %%.1200e huge precision (1206 chars)\n");
		} else {
			tests_failed++;
			printf("FAIL: %%.1200e huge precision: n=%d\n", n);
			if (n > 0 && n < (int) sizeof(big)) {
				printf("      [0..8]     = '%.*s'\n", 9, big);
				printf("      [1198..1206] = '%.*s'\n", 9, big + 1198);
			}
		}

		/* Integer path: %1500d was already safe before the rework
		 * (its padding always used OUT), but including it verifies
		 * that the shared buffer[] sizing is still correct. */
		memset(big, 0, sizeof(big));
		n = snprintf(big, sizeof(big), "%1500d", 42);
		tests_run++;
		if (n == 1500 &&
				big[0] == ' ' && big[1497] == ' ' &&
				big[1498] == '4' && big[1499] == '2' &&
				big[1500] == '\0') {
			tests_passed++;
			printf("OK: %%1500d integer (1500 chars)\n");
		} else {
			tests_failed++;
			printf("FAIL: %%1500d integer: n=%d\n", n);
		}
	}
	printf("\n");

	/* ==================================================================
	 * Additional
	 * ================================================================== */
	// --- DOUBLE TESTS
	TESTFMT("f round dp|0.0010000|", "%.7f", 0.000999955);
	TESTFMT("f round int|10.0000000|", "%.7f", 9.999999955);
	TESTFMT("f small half|0.0000001|", "%.7f", 0.000000055);
	TESTFMT("f big half|123456790.0000000|", "%.7f", 123456789.999999955);

	// --- LONG DOUBLE TESTS
	TESTFMT("ld |1.000000000000000001|", "%.18Lf", 1.000000000000000001L);
	TESTFMT("ld near1|1.00000000000000000000|", "%.20Lf", 0.999999999999999999995L);
	TESTFMT("ld near2|2.00000000000000000000|", "%.20Lf", 1.999999999999999999995L);
	TESTFMT("ld near05|0.50000000000000000000|", "%.20Lf", 0.499999999999999999995L);
	TESTFMT("ld dp|0.00100000000000000000|", "%.20Lf", 0.00099999999999999999995L);

	TESTFMT("ld int|10.00000000000000000000|", "%.20Lf", 9.999999999999999999995L);

	// --- special tests
	TESTFMT("ld small|0.00000000000000000010|", "%.20Lf", 0.000000000000000000095L);
	TESTFMT("ld big|123456790.0000000000|", "%.10Lf", 123456789.99999999995L);
	TESTFMT("ld exp|1.234567890123456789e+00|", "%.18Le", 1.234567890123456789L);

	printf("\n");
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
		if (n == 11 && strcmp(tb, "hello w") == 0) {
			tests_passed++;
			printf("OK: truncation returns full length\n");
		}
		else {
			tests_failed++;
			printf("FAIL: truncation returns full length: n=%d tb=\"%s\"\n", n, tb);
		}
		memset(tb, 'x', sizeof(tb));
		n = snprintf(tb, 0, "abc");
		tests_run++;
		if (n == 3 && tb[0] == 'x') {
			tests_passed++;
			printf("OK: size 0 writes nothing, returns length\n");
		}
		else {
			tests_failed++;
			printf("FAIL: size 0: n=%d tb[0]=%c\n", n, tb[0]);
		}
		n = snprintf(NULL, 0, "%d/%s", 42, "xy");
		tests_run++;
		if (n == 5) {
			tests_passed++;
			printf("OK: (NULL, 0) sizing idiom\n");
		}
		else {
			tests_failed++;
			printf("FAIL: (NULL, 0) sizing idiom: n=%d\n", n);
		}
		n = snprintf(tb, 7, "abcdef");
		tests_run++;
		if (n == 6 && strcmp(tb, "abcdef") == 0) {
			tests_passed++;
			printf("OK: exact fit\n");
		}
		else {
			tests_failed++;
			printf("FAIL: exact fit: n=%d tb=\"%s\"\n", n, tb);
		}
		/* errno: a filled string buffer sets EPERM inside __swbuf; a
		 * successful truncating call must not leak that to the caller. */
		errno = 0;
		n = snprintf(tb, sizeof(tb), "hello world");
		tests_run++;
		if (n == 11 && errno == 0) {
			tests_passed++;
			printf("OK: truncation leaves errno untouched\n");
		}
		else {
			tests_failed++;
			printf("FAIL: truncation errno: n=%d errno=%d\n", n, errno);
		}
		errno = 4242;
		n = snprintf(NULL, 0, "%d", 7);
		tests_run++;
		if (n == 1 && errno == 4242) {
			tests_passed++;
			printf("OK: sizing preserves a caller's errno\n");
		}
		else {
			tests_failed++;
			printf("FAIL: sizing errno: n=%d errno=%d (want 1/4242)\n", n, errno);
		}
	}

	/* ==================================================================
	 * Summary
	 * ================================================================== */
	printf("=== Test Summary ===\n");
	printf("  Run:     %d\n", tests_run);
	printf("  Passed:  %d\n", tests_passed);
	printf("  Failed:  %d\n", tests_failed);
	printf("\n");
	return tests_failed > 0 ? 1 : 0;
}
