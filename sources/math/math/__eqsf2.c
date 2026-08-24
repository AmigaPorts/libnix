#include <proto/mathieeesingbas.h>

asm(".globl ___eqsf2; ___eqsf2 = ___cmpsf2");
asm(".globl ___nesf2; ___nesf2 = ___cmpsf2");
asm(".globl ___ltsf2; ___ltsf2 = ___cmpsf2");
asm(".globl ___lesf2; ___lesf2 = ___cmpsf2");
asm(".globl ___gtsf2; ___gtsf2 = ___cmpsf2");
asm(".globl ___gesf2; ___gesf2 = ___cmpsf2");
signed long __cmpsf2(float x,float y)
{ return IEEESPCmp(x,y); }
