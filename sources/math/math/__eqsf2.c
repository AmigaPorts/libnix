/* Soft-float float compares, done on the bits rather than through
   IEEESPCmp(): the library call costs more than the comparison itself.
   IEEE floats order like sign-magnitude integers, so negate the magnitude
   of a negative one and compare as integers.  +0 and -0 are equal.  An
   unordered compare must make ==, <, <=, > and >= false, so with a NaN
   operand __eqsf2/__nesf2 and __ltsf2/__lesf2 return 1 and
   __gtsf2/__gesf2 return -1, as libgcc specifies.  */

asm(".globl ___eqsf2; ___eqsf2 = ___lesf2");
asm(".globl ___nesf2; ___nesf2 = ___lesf2");
asm(".globl ___ltsf2; ___ltsf2 = ___lesf2");
asm(".globl ___cmpsf2; ___cmpsf2 = ___lesf2");
asm(".globl ___gtsf2; ___gtsf2 = ___gesf2");

signed long __lesf2(float x, float y)
{
	union { float f; unsigned long l; } ux = { x }, uy = { y };
	long kx = ux.l & 0x7fffffff, ky = uy.l & 0x7fffffff;

	if (kx > 0x7f800000 || ky > 0x7f800000)
		return 1;
	if (ux.l & 0x80000000)
		kx = -kx;
	if (uy.l & 0x80000000)
		ky = -ky;
	return kx < ky ? -1 : kx > ky;
}

/* x >= y is y <= x, with the result negated; a NaN gives -1.  */
signed long __gesf2(float x, float y)
{
	return -__lesf2(y, x);
}
