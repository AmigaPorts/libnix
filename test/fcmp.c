/* Soft-float comparisons: with a NaN operand all are false except !=,
   +0 equals -0, and ordinary, negative, denormal and huge values order.
   The values are read from volatile variables so that gcc cannot fold the
   comparisons and has to call __eqdf2 and friends.  */

#include <stdio.h>

static int failures;

#define CHECK(expr, want) do { \
	if ((expr) != (want)) { \
		printf("FAIL: %s is %d\n", #expr, !(want)); \
		failures++; \
	} } while (0)

volatile double vdnan = __builtin_nan(""), vdone = 1.0, vdtwo = 2.0;
volatile double vdinf = __builtin_inf();
volatile float vfnan = __builtin_nanf(""), vfone = 1.0f, vftwo = 2.0f;
volatile float vfinf = __builtin_inff();
volatile double vdzero = 0.0, vdnzero = -0.0, vdm1 = -1.0, vdm2 = -2.0;
volatile double vdden = 4.9e-324, vdbig = 1.7e308;
volatile float vfzero = 0.0f, vfnzero = -0.0f, vfm1 = -1.0f, vfm2 = -2.0f;
volatile float vfden = 1.4e-45f, vfbig = 3.4e38f;

int main(void)
{
	double n = vdnan, one = vdone, two = vdtwo, inf = vdinf;
	float nf = vfnan, onef = vfone, twof = vftwo, inff = vfinf;
	double z = vdzero, nz = vdnzero, m1 = vdm1, m2 = vdm2;
	double den = vdden, big = vdbig;
	float zf = vfzero, nzf = vfnzero, m1f = vfm1, m2f = vfm2;
	float denf = vfden, bigf = vfbig;

	CHECK(n == one, 0); CHECK(n != one, 1);
	CHECK(n < one, 0); CHECK(n <= one, 0);
	CHECK(n > one, 0); CHECK(n >= one, 0);
	CHECK(one < n, 0); CHECK(one >= n, 0);
	CHECK(n == n, 0); CHECK(n != n, 1);
	CHECK(n < inf, 0); CHECK(n > -inf, 0);
	CHECK(one == one, 1); CHECK(one < two, 1);
	CHECK(two > one, 1); CHECK(one <= one, 1);
	CHECK(two >= two, 1); CHECK(one != two, 1);
	CHECK(inf > two, 1); CHECK(-inf < one, 1);
	CHECK(z == nz, 1); CHECK(z != nz, 0); CHECK(nz < z, 0);
	CHECK(nz <= z, 1); CHECK(nz >= z, 1);
	CHECK(m2 < m1, 1); CHECK(m1 > m2, 1); CHECK(m1 < one, 1);
	CHECK(m1 == m1, 1); CHECK(m1 >= m2, 1); CHECK(m2 > m1, 0);
	CHECK(den > z, 1); CHECK(-den < nz, 1); CHECK(den < one, 1);
	CHECK(big < inf, 1); CHECK(-big > -inf, 1); CHECK(big > two, 1);

	CHECK(nf == onef, 0); CHECK(nf != onef, 1);
	CHECK(nf < onef, 0); CHECK(nf <= onef, 0);
	CHECK(nf > onef, 0); CHECK(nf >= onef, 0);
	CHECK(onef < nf, 0); CHECK(onef >= nf, 0);
	CHECK(nf == nf, 0); CHECK(nf != nf, 1);
	CHECK(nf < inff, 0); CHECK(nf > -inff, 0);
	CHECK(onef == onef, 1); CHECK(onef < twof, 1);
	CHECK(twof > onef, 1); CHECK(onef <= onef, 1);
	CHECK(twof >= twof, 1); CHECK(onef != twof, 1);
	CHECK(inff > twof, 1); CHECK(-inff < onef, 1);
	CHECK(zf == nzf, 1); CHECK(zf != nzf, 0); CHECK(nzf < zf, 0);
	CHECK(nzf <= zf, 1); CHECK(nzf >= zf, 1);
	CHECK(m2f < m1f, 1); CHECK(m1f > m2f, 1); CHECK(m1f < onef, 1);
	CHECK(m1f == m1f, 1); CHECK(m1f >= m2f, 1); CHECK(m2f > m1f, 0);
	CHECK(denf > zf, 1); CHECK(-denf < nzf, 1); CHECK(denf < onef, 1);
	CHECK(bigf < inff, 1); CHECK(-bigf > -inff, 1); CHECK(bigf > twof, 1);

	if (failures == 0)
		printf("fcmp: all passed\n");
	return failures != 0;
}
