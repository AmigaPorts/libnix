/* Soft-float double compares, done on the bits rather than through
   IEEEDPCmp(): the library call costs more than the comparison itself.
   IEEE doubles order like sign-magnitude integers, so negate the
   magnitude of a negative one and compare as 64-bit integers.  +0 and -0
   are equal.  An unordered compare must make ==, <, <=, > and >= false,
   so with a NaN operand __eqdf2/__nedf2 and __ltdf2/__ledf2 return 1 and
   __gtdf2/__gedf2 return -1, as libgcc specifies.  */

asm(".globl ___eqdf2; ___eqdf2 = ___ledf2");
asm(".globl ___nedf2; ___nedf2 = ___ledf2");
asm(".globl ___ltdf2; ___ltdf2 = ___ledf2");
asm(".globl ___cmpdf2; ___cmpdf2 = ___ledf2");
asm(".globl ___gtdf2; ___gtdf2 = ___gedf2");

signed long __ledf2(double x, double y)
{
	union { double d; unsigned long l[2]; } ux = { x }, uy = { y };
	unsigned long ax = ux.l[0] & 0x7fffffff, ay = uy.l[0] & 0x7fffffff;
	long long kx, ky;

	if (ax > 0x7ff00000 || (ax == 0x7ff00000 && ux.l[1] != 0)
	    || ay > 0x7ff00000 || (ay == 0x7ff00000 && uy.l[1] != 0))
		return 1;
	kx = (long long)((unsigned long long)ax << 32 | ux.l[1]);
	ky = (long long)((unsigned long long)ay << 32 | uy.l[1]);
	if (ux.l[0] & 0x80000000)
		kx = -kx;
	if (uy.l[0] & 0x80000000)
		ky = -ky;
	return kx < ky ? -1 : kx > ky;
}

/* x >= y is y <= x, with the result negated; a NaN gives -1.  */
signed long __gedf2(double x, double y)
{
	return -__ledf2(y, x);
}
