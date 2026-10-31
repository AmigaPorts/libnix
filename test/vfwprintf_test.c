/* ==========================================================================
 *  vfwprintf_test.c
 *
 *  Self-test harness for the wide printf engine.
 *  Build with -DFULL_SPECIFIERS and link against
 *  __vfwprintf_total_size.o and ami_printf_math.o.
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
#include <wchar.h>
#include <string.h>

#include "ami_printf_math.h"

/* Test counters for the wide framework execution */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/*
 * Check that the wide formatter output matches the expected wide string exactly.
 * The 'combined' string uses the format: L"label|expected_result|"
 */
static int check_fmt(const wchar_t *combined, const wchar_t *fmt, ...) {
	wchar_t buf[512];
	const wchar_t *expected;
	const wchar_t *label;
	const wchar_t *last_sep;
	const wchar_t *first_sep;
	size_t expected_len;
	int ret;
	va_list args;

	/* Find the separators to isolate the label and expected string slice */
	last_sep = wcsrchr(combined, L'|');
	if (!last_sep) {
		wprintf(L"BUG: no '|' in test spec: %ls\n", combined);
		tests_run++;
		tests_failed++;
		return 0;
	}

	first_sep = wcschr(combined, L'|');
	if (first_sep == last_sep) {
		label = L"(unnamed)";
		expected = combined;
		expected_len = (size_t) (last_sep - combined);
	} else {
		label = combined;
		expected = first_sep + 1;
		expected_len = (size_t) (last_sep - expected);
	}

	/* Execute the format engine via the internal wide vararg pipeline */
	va_start(args, fmt);
	ret = vswprintf(buf, sizeof(buf) / sizeof(wchar_t), fmt, args);
	va_end(args);

	tests_run++;
	if (ret == (int) expected_len && memcmp(buf, expected, expected_len * sizeof(wchar_t)) == 0) {
		wprintf(L"OK: %ls\n", label);
		tests_passed++;
		return 1;
	} else {
		tests_failed++;
		wprintf(L"FAIL: %ls\n", label);
		wprintf(L"      fmt:      \"%ls\"\n", fmt);
		wprintf(L"      expected: \"");
		/* Note: writing wide strings directly to stdout */
		for (size_t i = 0; i < expected_len; i++) wprintf(L"%lc", expected[i]);
		wprintf(L"\" (len=%zu)\n", expected_len);
		wprintf(L"      got:      \"");
		for (int i = 0; i < ret; i++) wprintf(L"%lc", buf[i]);
		wprintf(L"\" (len=%d)\n", ret);
		return 0;
	}
}

#define TESTFMT(desc, fmt, ...) \
    check_fmt(desc, fmt, ##__VA_ARGS__)

int main(int argc, char **argv) {
	double d;
	(void) argc;
	(void) argv;

	wprintf(L"=== wfprintf/vswprintf 64-bit validation suite ===\n\n");

	/* ==================================================================
	 * SECTION 1: Zero and alternate-form tests
	 * ================================================================== */
	wprintf(L"--- Zero / alternate-form tests ---\n");
	TESTFMT(L"zero fixed left pad|0                   |", L"%-20.0f", 0.0);
	TESTFMT(L"zero sci left pad|0e+00               |", L"%-20.0e", 0.0);
	TESTFMT(L"zero g left pad|0                   |", L"%-20.1g", 0.0);
	TESTFMT(L"zero fixed alt right pad|                  0.|", L"%#20.0f", 0.0);
	TESTFMT(L"zero sci alt right pad|              0.e+00|", L"%#20.0e", 0.0);
	TESTFMT(L"zero g alt right pad|                 0.0|", L"%#20.1g", 0.0);
	TESTFMT(L"negative zero|-0|", L"%.0f", -0.0);
	TESTFMT(L"f tiny rounds to zero|0.000000|", L"%f", 6e-9);
	TESTFMT(L"f tiny rounds up|0.000001|", L"%f", 6e-7);
	TESTFMT(L"zero with plus|+0|", L"%+.0f", 0.0);
	TESTFMT(L"zero with space| 0|", L"% .0f", 0.0);
	TESTFMT(L"zero prec 5|0.00000|", L"%.5f", 0.0);
	TESTFMT(L"simple 5.0|5|", L"%.0f", 5.0);
	wprintf(L"\n");

	/* ==================================================================
	 * SECTION 2: Fixed-point (%f) decade walk
	 * ================================================================== */
	wprintf(L"--- Fixed-point %%f decade walk ---\n");
	d = 1.2345678902468e-13L;
	TESTFMT(L"f e-13|           0.0000000|", L"%20.7f", d);
	d = 1.2345678902468e-12L;
	TESTFMT(L"f e-12|           0.0000000|", L"%20.7f", d);
	d = 1.2345678902468e-11L;
	TESTFMT(L"f e-11|           0.0000000|", L"%20.7f", d);
	d = 1.2345678902468e-10L;
	TESTFMT(L"f e-10|           0.0000000|", L"%20.7f", d);
	d = 1.2345678902468e-09L;
	TESTFMT(L"f e-9 |           0.0000000|", L"%20.7f", d);
	d = 1.2345678902468e-08L;
	TESTFMT(L"f e-8 |           0.0000000|", L"%20.7f", d);
	d = 1.2345678902468e-07L;
	TESTFMT(L"f e-7 |           0.0000001|", L"%20.7f", d);
	d = 1.2345678902468e-06L;
	TESTFMT(L"f e-6 |           0.0000012|", L"%20.7f", d);
	d = 1.2345678902468e-05L;
	TESTFMT(L"f e-5 |           0.0000123|", L"%20.7f", d);
	d = 1.2345678902468e-04L;
	TESTFMT(L"f e-4 |           0.0001235|", L"%20.7f", d);
	d = 1.2345678902468e-03L;
	TESTFMT(L"f e-3 |           0.0012346|", L"%20.7f", d);
	d = 1.2345678902468e-02L;
	TESTFMT(L"f e-2 |           0.0123457|", L"%20.7f", d);
	d = 1.2345678902468e-01L;
	TESTFMT(L"f e-1 |           0.1234568|", L"%20.7f", d);
	d = 1.2345678902468e+00L;
	TESTFMT(L"f e0  |           1.2345679|", L"%20.7f", d);
	d = 1.2345678902468e+01L;
	TESTFMT(L"f e1  |          12.3456789|", L"%20.7f", d);
	d = 1.2345678902468e+02L;
	TESTFMT(L"f e2  |         123.4567890|", L"%20.7f", d);
	d = 1.2345678902468e+03L;
	TESTFMT(L"f e3  |        1234.5678902|", L"%20.7f", d);
	d = 1.2345678902468e+04L;
	TESTFMT(L"f e4  |       12345.6789025|", L"%20.7f", d);
	d = 1.2345678902468e+05L;
	TESTFMT(L"f e5  |      123456.7890247|", L"%20.7f", d);
	d = 1.2345678902468e+06L;
	TESTFMT(L"f e6  |     1234567.8902468|", L"%20.7f", d);
	d = 1.2345678902468e+07L;
	TESTFMT(L"f e7  |    12345678.9024680|", L"%20.7f", d);
	d = 1.2345678902468e+08L;
	TESTFMT(L"f e8  |   123456789.0246800|", L"%20.7f", d);
	d = 1.2345678902468e+09L;
	TESTFMT(L"f e9  |  1234567890.2467999|", L"%20.7f", d);
	d = 1.2345678902468e+10L;
	TESTFMT(L"f e10 | 12345678902.4680000|", L"%20.7f", d);
	wprintf(L"\n");

	/* ==================================================================
	 * SECTION 3: General (%g) decade walk
	 * ================================================================== */
	wprintf(L"--- General %%g decade walk ---\n");
	d = 1.2345678902468e-13L;
	TESTFMT(L"g e-13|        1.234568e-13|", L"%20.7g", d);
	d = 1.2345678902468e-12L;
	TESTFMT(L"g e-12|        1.234568e-12|", L"%20.7g", d);
	d = 1.2345678902468e-11L;
	TESTFMT(L"g e-11|        1.234568e-11|", L"%20.7g", d);
	d = 1.2345678902468e-10L;
	TESTFMT(L"g e-10|        1.234568e-10|", L"%20.7g", d);
	d = 1.2345678902468e-09L;
	TESTFMT(L"g e-9 |        1.234568e-09|", L"%20.7g", d);
	d = 1.2345678902468e-08L;
	TESTFMT(L"g e-8 |        1.234568e-08|", L"%20.7g", d);
	d = 1.2345678902468e-07L;
	TESTFMT(L"g e-7 |        1.234568e-07|", L"%20.7g", d);
	d = 1.2345678902468e-06L;
	TESTFMT(L"g e-6 |        1.234568e-06|", L"%20.7g", d);
	d = 1.2345678902468e-05L;
	TESTFMT(L"g e-5 |        1.234568e-05|", L"%20.7g", d);
	d = 1.2345678902468e-04L;
	TESTFMT(L"g e-4 |        0.0001234568|", L"%20.7g", d);
	d = 1.2345678902468e-03L;
	TESTFMT(L"g e-3 |         0.001234568|", L"%20.7g", d);
	d = 1.2345678902468e-02L;
	TESTFMT(L"g e-2 |          0.01234568|", L"%20.7g", d);
	d = 1.2345678902468e-01L;
	TESTFMT(L"g e-1 |           0.1234568|", L"%20.7g", d);
	d = 1.2345678902468e+00L;
	TESTFMT(L"g e0  |            1.234568|", L"%20.7g", d);
	d = 1.2345678902468e+01L;
	TESTFMT(L"g e1  |            12.34568|", L"%20.7g", d);
	d = 1.2345678902468e+02L;
	TESTFMT(L"g e2  |            123.4568|", L"%20.7g", d);
	d = 1.2345678902468e+03L;
	TESTFMT(L"g e3  |            1234.568|", L"%20.7g", d);
	d = 1.2345678902468e+04L;
	TESTFMT(L"g e4  |            12345.68|", L"%20.7g", d);
	d = 1.2345678902468e+05L;
	TESTFMT(L"g e5  |            123456.8|", L"%20.7g", d);
	d = 1.2345678902468e+06L;
	TESTFMT(L"g e6  |             1234568|", L"%20.7g", d);
	d = 1.2345678902468e+07L;
	TESTFMT(L"g e7  |        1.234568e+07|", L"%20.7g", d);
	d = 1.2345678902468e+08L;
	TESTFMT(L"g e8  |        1.234568e+08|", L"%20.7g", d);
	d = 1.2345678902468e+09L;
	TESTFMT(L"g e9  |        1.234568e+09|", L"%20.7g", d);
	d = 1.2345678902468e+10L;
	TESTFMT(L"g e10 |        1.234568e+10|", L"%20.7g", d);
	wprintf(L"\n");

	/* ==================================================================
	 * SECTION 4: Scientific (%e) decade walk
	 * ================================================================== */
	wprintf(L"--- Scientific %%e decade walk ---\n");
	d = 1.2345678902468e-13L;
	TESTFMT(L"e e-13|       1.2345679e-13|", L"%20.7e", d);
	d = 1.2345678902468e-12L;
	TESTFMT(L"e e-12|       1.2345679e-12|", L"%20.7e", d);
	d = 1.2345678902468e-11L;
	TESTFMT(L"e e-11|       1.2345679e-11|", L"%20.7e", d);
	d = 1.2345678902468e-10L;
	TESTFMT(L"e e-10|       1.2345679e-10|", L"%20.7e", d);
	d = 1.2345678902468e-09L;
	TESTFMT(L"e e-9 |       1.2345679e-09|", L"%20.7e", d);
	d = 1.2345678902468e-08L;
	TESTFMT(L"e e-8 |       1.2345679e-08|", L"%20.7e", d);
	d = 1.2345678902468e-07L;
	TESTFMT(L"e e-7 |       1.2345679e-07|", L"%20.7e", d);
	d = 1.2345678902468e-06L;
	TESTFMT(L"e e-6 |       1.2345679e-06|", L"%20.7e", d);
	d = 1.2345678902468e-05L;
	TESTFMT(L"e e-5 |       1.2345679e-05|", L"%20.7e", d);
	d = 1.2345678902468e-04L;
	TESTFMT(L"e e-4 |       1.2345679e-04|", L"%20.7e", d);
	d = 1.2345678902468e-03L;
	TESTFMT(L"e e-3 |       1.2345679e-03|", L"%20.7e", d);
	d = 1.2345678902468e-02L;
	TESTFMT(L"e e-2 |       1.2345679e-02|", L"%20.7e", d);
	d = 1.2345678902468e-01L;
	TESTFMT(L"e e-1 |       1.2345679e-01|", L"%20.7e", d);
	d = 1.2345678902468e+00L;
	TESTFMT(L"e e0  |       1.2345679e+00|", L"%20.7e", d);
	d = 1.2345678902468e+01L;
	TESTFMT(L"e e1  |       1.2345679e+01|", L"%20.7e", d);
	d = 1.2345678902468e+02L;
	TESTFMT(L"e e2  |       1.2345679e+02|", L"%20.7e", d);
	d = 1.2345678902468e+03L;
	TESTFMT(L"e e3  |       1.2345679e+03|", L"%20.7e", d);
	d = 1.2345678902468e+04L;
	TESTFMT(L"e e4  |       1.2345679e+04|", L"%20.7e", d);
	d = 1.2345678902468e+05L;
	TESTFMT(L"e e5  |       1.2345679e+05|", L"%20.7e", d);
	d = 1.2345678902468e+06L;
	TESTFMT(L"e e6  |       1.2345679e+06|", L"%20.7e", d);
	d = 1.2345678902468e+07L;
	TESTFMT(L"e e7  |       1.2345679e+07|", L"%20.7e", d);
	d = 1.2345678902468e+08L;
	TESTFMT(L"e e8  |       1.2345679e+08|", L"%20.7e", d);
	d = 1.2345678902468e+09L;
	TESTFMT(L"e e9  |       1.2345679e+09|", L"%20.7e", d);
	d = 1.2345678902468e+10L;
	TESTFMT(L"e e10 |       1.2345679e+10|", L"%20.7e", d);
	wprintf(L"\n");

	/* ==================================================================
	 * SECTION 5: Precision boundaries (Wide strings)
	 * ================================================================== */
	wprintf(L"--- Precision and Edge-case tests ---\n");
	TESTFMT(L"f round dp|0.0010000|", L"%.7f", 0.000999955);
	TESTFMT(L"f round int|10.0000000|", L"%.7f", 9.999999955);
	TESTFMT(L"f small half|0.0000001|", L"%.7f", 0.000000055);
	TESTFMT(L"f big half|123456790.0000000|", L"%.7f", 123456789.999999955);
	TESTFMT(L"f huge prec|1.2345678901234567|", L"%.16f", 1.23456789012345679);
	TESTFMT(L"ld near1|1.00000000000000000000|", L"%.20Lf", 0.999999999999999999995L);
	TESTFMT(L"ld near2|2.00000000000000000000|", L"%.20Lf", 1.999999999999999999995L);
	TESTFMT(L"ld near05|0.50000000000000000000|", L"%.20Lf", 0.499999999999999999995L);
	TESTFMT(L"ld dp|0.00100000000000000000|", L"%.20Lf", 0.00099999999999999999995L);
	TESTFMT(L"ld int|10.00000000000000000000|", L"%.20Lf", 9.999999999999999999995L);
	TESTFMT(L"ld small|0.00000000000000000010|", L"%.20Lf", 0.000000000000000000095L);
	TESTFMT(L"ld big|123456789.9999999999|", L"%.10Lf", 123456789.99999999995L);
	long double pi_ld = 3.141592653589793238L;
	TESTFMT(L"ld high prec|3.141592653589793238|", L"%.18Lf", pi_ld);
	wprintf(L"\n");
	/* ==================================================================*
	 * Summary Output*
	 * ================================================================== */
	wprintf(L"=== Test Summary ===\n");
	wprintf(L"  Run:     %d\n", tests_run);
	wprintf(L"  Passed:  %d\n", tests_passed);
	wprintf(L"  Failed:  %d\n", tests_failed);
	wprintf(L"\n");
	return tests_failed > 0 ? 1 : 0;
}
