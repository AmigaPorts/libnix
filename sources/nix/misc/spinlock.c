/*
 * spinlock.c - Slow path for AmigaOS / m68k spinlocks
 *
 * The slow path is entered only when the atomic test-and-set operation
 * detects an already-held lock.
 *
 * Instead of continuously spinning at the current priority, the waiting
 * task temporarily lowers its priority and calls Permit(). This allows
 * the scheduler to run the lock owner, which can then release the lock.
 *
 * The original priority is restored before retrying the atomic operation.
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
 */

#include "spinlock.h"

/*
 * Waits until the spinlock can be acquired.
 *
 * Temporarily lowers the current task's priority and yields through
 * Permit() whenever another task holds the lock.
 */
void __slowSpinLock(spinlock_t *l asm("a0")) {
    register struct ExecBase *SysBase asm("a6") = *(struct ExecBase **)4;
    struct Task *dis = SysBase->ThisTask;

    for (;;) {
        BYTE locked;

        __asm__ volatile (
            "bset #0, %0\n\t"
            "sne %1"
            : "+m" (3[(char *)l]), "=d" (locked)
            :
            : "cc", "memory"
        );

        if (!locked)
            break;

        dis->tc_Node.ln_Pri -= 5;     /* Lower priority a bit. */
        SysBase->SysFlags |= 1 << 15; /* Tell Permit() to reschedule. */
        dis->tc_TDNestCnt = 1;        /* Fake Forbid. */
        Permit();                     /* Allow the lock owner to run. */
        dis->tc_Node.ln_Pri += 5;     /* Restore priority. */
    }
}
