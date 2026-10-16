/* frexpl() on the m68k extended format, including denormals. */
#include <stdio.h>
#include <string.h>
#include <float.h>

long double frexpl(long double, int *);

static int fails;

static long double rebuild(long double m, int e)
{
	while (e > 0) { m *= 2; e--; }
	while (e < 0) { m /= 2; e++; }
	return m;
}

static void hex(const char *tag, long double v)
{
	unsigned char b[12];
	memcpy(b, &v, 12);
	printf("%s %02x%02x %02x%02x%02x%02x%02x%02x%02x%02x", tag, b[0], b[1],
	       b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11]);
}

/* denormals: gcc folds these constants with the m68k format rules, so
   they are an oracle independent of run-time arithmetic, whose emulation
   (libgcc soft float, volamos FPU) scales an exponent field of 0 like x87 */
static void expect(const char *name, long double x, int want_e, long double want_m)
{
	int e;
	long double m = frexpl(x, &e);
	if (e != want_e || memcmp(&m, &want_m, 12) != 0) {
		fails++;
		printf("FAIL %s e=%d want %d", name, e, want_e);
		hex(" x", x);
		hex(" m", m);
		printf("\n");
	}
}

static void check(const char *name, long double x)
{
	int e;
	long double m = frexpl(x, &e);
	int bad = 0;

	if (x != x) {
		bad = m == m || e != 0;
	} else if (x == 0 || x - x != 0) {	/* zero or infinity */
		bad = memcmp(&m, &x, 12) != 0 || e != 0;
	} else {
		long double a = m < 0 ? -m : m;
		bad = a < 0.5L || a >= 1.0L || rebuild(m, e) != x;
#ifdef __HAVE_68881__
		{
			long double fe;
			__asm__("fgetexp.x %1,%0" : "=f"(fe) : "f"(x));
			if ((int)fe + 1 != e)
				bad = 1;
		}
#endif
	}
	if (bad) {
		fails++;
		printf("FAIL %s e=%d", name, e);
		hex(" x", x);
		hex(" m", m);
		printf("\n");
	}
}

int main(void)
{
	volatile long double zero = 0, one = 1;
	long double inf = one / zero, nan = zero / zero;
	long double d;
	int i;

	check("1", 1.0L);
	check("0.5", 0.5L);
	check("-3", -3.0L);
	check("0.1", 0.1L);
	check("1e4000", 1e4000L);
	check("-1e-4000", -1e-4000L);
	check("LDBL_MAX", LDBL_MAX);
	check("-LDBL_MAX", -LDBL_MAX);
	expect("LDBL_MIN", LDBL_MIN, -16382, 0.5L);
	expect("0x1p-16383L", 0x1p-16383L, -16382, 0.5L);
	expect("0x1p-16384L", 0x1p-16384L, -16383, 0.5L);
	expect("0x1.8p-16400L", 0x1.8p-16400L, -16399, 0.75L);
	expect("-0x1.fp-16420L", -0x1.fp-16420L, -16419, -0x1.fp-1L);
	expect("LDBL_TRUE_MIN", LDBL_TRUE_MIN, -16445, 0.5L);
	expect("-5*LDBL_TRUE_MIN", -0x1.4p-16444L, -16443, -0.625L);
	expect("0x1.8p-16382L", 0x1.8p-16382L, -16381, 0.75L);
	check("0", zero);
	check("-0", -zero);
	check("inf", inf);
	check("-inf", -inf);
	check("nan", nan);
	for (d = LDBL_MIN * 2, i = 0; i < 70; i++, d *= 3)
		check("small sweep", d);
	for (d = 1, i = 0; i < 200; i++, d *= 7.25L)
		check("sweep", d);
	printf("%s: %d failures\n", fails ? "FAIL" : "ok", fails);
	return fails != 0;
}
