/*
 * swapstack.c
 *
 * A libnix startup module that moves the program to a bigger stack when
 * the one it was started with is smaller than __stack.
 *
 * Code derived from a stackswap module by Kriton Kyrimis (kyrimis@theseas.ntua.gr)
 *
 * Usage: set __stack and reference __stkinit, which pulls this module in;
 * ADD2INIT runs it before main() and __stkexit moves back at exit():
 *
 *   extern void __stkinit(void);
 *   void *__stkinit_ref = __stkinit;
 *   unsigned long __stack = YOUR_STACK_SIZE;
 */
#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <stdlib.h>

// force using the section based init
#define __libnix__ 1

#include "stabs.h"

extern struct ExecBase *SysBase;
extern UWORD *__SaveSP;
extern unsigned long __stack;
void __request(const char *text);

#if defined(__KICK13__)
extern void * AllocVec(unsigned, int);
extern void FreeVec(void *);
#endif

/*
 * Switch the calling task to the stack described by sss: stk_Lower and
 * stk_Upper are the new bounds, stk_Pointer is where the top of the
 * copied region goes. The live part of the current stack, from top
 * (exclusive) down to the caller's sp, is copied below stk_Pointer so that
 * the caller and its callers keep their data at the same sp-relative
 * offsets. Returns on the new stack with every callee-saved register
 * preserved; sss receives the old bounds and the old sp.
 *
 * Written in asm: between the copy and the switch no compiler-generated
 * stack access may happen, and the saved registers must be restored from
 * the copy.
 */
void __stkswap(struct StackSwapStruct *sss asm("a0"), char *top asm("a1"),
	       struct ExecBase *SysBase asm("a6"));

asm(
"	.text\n"
"	.even\n"
"	.globl	___stkswap\n"
"___stkswap:\n"
"	moveml	d2/a2/a3,sp@-\n"	/* saved on the old stack, restored from the copy */
"	movel	a0,a2\n"		/* sss */
"	movel	a1,d2\n"		/* top of the region to copy */
"	movel	a6@(276),a3\n"		/* ThisTask */
"	jsr	a6@(-120)\n"		/* Disable() */
"	movel	a2@,d0\n"		/* exchange stk_Lower/stk_Upper with the task's bounds */
"	movel	a3@(58),a2@\n"		/* tc_SPLower */
"	movel	d0,a3@(58)\n"
"	movel	a2@(4),d0\n"
"	movel	a3@(62),a2@(4)\n"	/* tc_SPUpper */
"	movel	d0,a3@(62)\n"
"	movel	a2@(8),a0\n"		/* stk_Pointer: destination top */
"	movel	sp,a2@(8)\n"		/* returns the old sp, like StackSwap() */
"	movel	d2,a1\n"
"1:	movew	a1@-,a0@-\n"		/* copy [sp, top) downwards */
"	cmpl	sp,a1\n"
"	jhi	1b\n"
"	movel	a0,sp\n"		/* continue on the new stack */
"	movel	a0,a3@(54)\n"		/* tc_SPReg */
"	jsr	a6@(-126)\n"		/* Enable() */
"	moveml	sp@+,d2/a2/a3\n"
"	rts\n"
);

/*
 * The frame pointer must stay off in __stkinit() and __stkexit(): a5
 * would keep pointing into the old stack across __stkswap(), and unlk
 * would return there.
 */
#pragma GCC push_options
#pragma GCC optimize ("-Os")
#pragma GCC optimize ("-fomit-frame-pointer")

static struct StackSwapStruct stack;
static char *newstack;
static long moved;	/* distance __SaveSP moved onto the new stack */

void __stkinit(void)
{
	struct Task *task = SysBase->ThisTask;
	ULONG needed = __stack;
	char *new;

#if defined(__KICK13__)
	task->tc_SPUpper = __SaveSP + 6;
#else
	if (needed <= (ULONG)((char *)task->tc_SPUpper - (char *)task->tc_SPLower))
		return;
#endif

	/* Round size to next long word */
	needed = (needed + (sizeof(LONG) - 1)) & ~(sizeof(LONG) - 1);

	newstack = new = AllocVec(needed, MEMF_PUBLIC);
	if (!new) {
		__request("Couldn't allocate new stack!");
		exit(RETURN_FAIL);
	}

	stack.stk_Lower = new;
	stack.stk_Upper = (ULONG)(new + needed);
	stack.stk_Pointer = (APTR)stack.stk_Upper;

	/*
	 * Everything from the entry sp saved by the startup code down to
	 * here moves to the top of the new stack. What lies above __SaveSP
	 * belongs to the caller of the program and stays where it is.
	 */
	moved = (char *)stack.stk_Pointer - (char *)__SaveSP;
	__stkswap(&stack, (char *)__SaveSP, SysBase);
	__SaveSP = (UWORD *)((char *)__SaveSP + moved);
}

void __stkexit(void)
{
	if (!newstack)
		return;

	/* Move the live part back to where it came from and free the new stack */
	stack.stk_Pointer = (char *)__SaveSP - moved;
	__stkswap(&stack, (char *)__SaveSP, SysBase);
	__SaveSP = (UWORD *)((char *)__SaveSP - moved);

	FreeVec(newstack);
	newstack = NULL;
}

#pragma GCC pop_options

/* The same priority as the detach module - you cannot use them both */
ADD2INIT(__stkinit, -70);
ADD2EXIT(__stkexit, -70);
