/*
 * spinlock.h - Lightweight spinlocks for AmigaOS / m68k
 *
 * These spinlocks are intended exclusively for very short critical sections.
 * They use an atomic 68000 bit-test-and-set instruction for the fast path.
 * Contended locks enter a slow path that yields the CPU, as sched_yield()
 * does, until the owner has released the lock.
 *
 * Recursive spinlocks additionally track the owning task and allow the
 * owner to acquire the same lock recursively.
 *
 * IMPORTANT:
 * - Critical sections must be short and must NOT call
 *   Delay(), Wait(), WaitPort(), WaitIO(), DoIO(), WaitTOF(),
 *   WaitBOVP(), WaitBlit(), ObtainSemaphore(),
 *   ObtainSemaphoreShared(), ObtainSemaphoreList(),
 *   ObtainMutex(), LockMutex().
 * - These locks are not intended for long-running operations.
 * - A lock must always be released by the task that acquired it.
 *   No ownership checks are performed for maximum performance.
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

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ULONG lock;
} spinlock_t;

typedef struct {
    union {
        spinlock_t lock;
        UWORD count;
    };
    ULONG owner;
} rspinlock_t;

/* Handles contention by yielding the CPU until the lock becomes available. */
extern void __slowSpinLock(spinlock_t *l asm("a0"));

/* Initializes a recursive spinlock to the unlocked state. */
static inline void __initRecursiveSpinLock(rspinlock_t *lock asm("a0")) {
    lock->lock.lock = 0;
    lock->owner = 0;
}

/* Acquires a spinlock using an atomic test-and-set operation.
 * Calls the slow path only if the lock is already held.
 */
static inline void __spinLock(spinlock_t *l asm("a0")) {
    BYTE locked;

    __asm__ volatile (
        "bset #0, %0\n\t"
        "sne %1"
        : "+m" (3[(char *)l]), "=d" (locked)
        :
        : "cc", "memory"
    );

    if (locked)
        __slowSpinLock(l);
}

/* Releases a spinlock and makes preceding memory accesses visible.
 * The fence is atomic_signal_fence(), by the name that needs no
 * <stdatomic.h>, which <stdio.h> must not include: it emits no code and
 * keeps the compiler from moving accesses across it, all that one CPU
 * needs.
 */
static inline void __spinUnlock(spinlock_t *l asm("a0")) {
    __atomic_signal_fence(__ATOMIC_SEQ_CST);
    l->lock = 0;
}

/* Acquires a recursive spinlock.
 * If the current task already owns the lock, only the recursion count
 * is incremented; otherwise the underlying spinlock is acquired.
 * __spinLock()'s bset is the barrier that keeps the locked section's
 * accesses after it.
 */
static inline void __recursiveSpinLock(rspinlock_t *l asm("a0")) {
    struct ExecBase *SysBase = *(struct ExecBase **) 4;
    struct Task *dis = SysBase->ThisTask;
    if (l->owner == (ULONG) dis) {
        ++l->count;
        return;
    }

    __spinLock(&l->lock);
    l->count = 1;
    l->owner = (ULONG) dis;
}

/* Releases one recursion level.
 * The owner is cleared only when the final recursion level is released.
 * The barrier keeps everything before it, the owner's clearing included,
 * ahead of the store that frees the lock: a later owner = 0 would erase
 * the next owner's.
 */
static inline void __recursiveSpinUnlock(rspinlock_t *l asm("a0")) {
    if (--l->count == 0) {
        l->owner = 0;
        __atomic_signal_fence(__ATOMIC_SEQ_CST);
        l->lock.lock = 0;
    }
}

#ifdef __cplusplus
}
#endif
#endif
