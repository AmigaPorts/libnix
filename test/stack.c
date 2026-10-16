/*
 * Target test for the swapstack module (sources/misc/swapstack.c): a
 * program that sets __stack must run on a stack at least that big, and
 * exit() must still find its way back to the original one.
 *
 * Run under vamos with a stack smaller than __stack, so that the swap
 * actually happens:  vamos -s 4 stack
 */
#include <exec/tasks.h>
#include <proto/exec.h>
#include <stdio.h>
#include <string.h>

#define WANTED (64UL * 1024)

/* Referencing __stkinit is what links the module in; ADD2INIT runs it. */
extern void __stkinit(void);
void *__stkinit_ref = __stkinit;
unsigned long __stack = WANTED;

/* 1 KiB per frame: 40 levels do not fit in the 4 KiB default stack. */
static int depth(int n)
{
    volatile char pad[1024];

    memset((char *)pad, n, sizeof(pad));
    return n ? depth(n - 1) + pad[sizeof(pad) - 1] : 0;
}

int main(void)
{
    struct Task *task = FindTask(NULL);
    char *lower = task->tc_SPLower, *upper = task->tc_SPUpper;
    char probe;
    int sum;

    if ((unsigned long)(upper - lower) < WANTED) {
        printf("FAIL: stack is %lu bytes, __stack asked for %lu\n",
               (unsigned long)(upper - lower), WANTED);
        return 1;
    }
    if (&probe < lower || &probe >= upper) {
        printf("FAIL: sp %p is outside the task stack %p..%p\n",
               (void *)&probe, (void *)lower, (void *)upper);
        return 1;
    }
    sum = depth(40);
    if (sum != 40 * 41 / 2) {
        printf("FAIL: recursion returned %d\n", sum);
        return 1;
    }
    printf("ok: %lu byte stack\n", (unsigned long)(upper - lower));
    return 0;
}
