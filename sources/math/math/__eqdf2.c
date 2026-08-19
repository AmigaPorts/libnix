#include <proto/mathieeedoubbas.h>

asm(".globl ___eqdf2; ___eqdf2 = ___cmpdf2");
asm(".globl ___nedf2; ___nedf2 = ___cmpdf2");
asm(".globl ___ltdf2; ___ltdf2 = ___cmpdf2");
asm(".globl ___ledf2; ___ledf2 = ___cmpdf2");
asm(".globl ___gtdf2; ___gtdf2 = ___cmpdf2");
asm(".globl ___gedf2; ___gedf2 = ___cmpdf2");
signed long __cmpdf2(double x,double y)
{
	return IEEEDPCmp(x,y);
}
