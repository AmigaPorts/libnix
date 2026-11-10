/*
 * spinlock.c - Slow path for AmigaOS / m68k spinlocks
 *
 * The slow path is entered only when the atomic test-and-set operation
 * detects an already-held lock.
 *
 * Between attempts, the waiting task ends its time slice, as sched_yield()
 * and gcc's __gthread_yield() do, so that the lock owner can run and
 * release the lock.
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
 * Waits until the spinlock can be acquired, yielding the CPU whenever
 * another task holds it.
 */
void __slowSpinLock(spinlock_t *l asm("a0")) {
    register struct ExecBase *SysBase asm("a6") = *(struct ExecBase **)4;

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

        /* End our time slice, as exec does when it runs out: a reschedule
           pending (bit 15) alone switches only to a higher-priority task,
           and with the slice over (bit 14) we also go behind the ready
           tasks of our own priority.  Atomically set both bits, since
           interrupts change SysFlags too. */
        Forbid();
        __asm__ volatile ("or.w #0xc000,%0" : "+m" (SysBase->SysFlags) : : "cc");
        Permit();
    }
}
