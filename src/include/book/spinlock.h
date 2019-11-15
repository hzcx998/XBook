/*
 * file:		   include/book/spinlock.h
 * auther:		Jason Hu
 * time:		   2019/11/15
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_SPINLOCK_H
#define _BOOK_SPINLOCK_H

#include <book/config.h>
#include <book/atomic.h>

//#define SPINLOCK_DEBUG

typedef struct Spinlock {
    Atomic_t lock; /* 锁变量是原子，为0表示空闲，大于等于0表示已经被使用 */
} Spinlock_t;

PUBLIC void SpinLockInit(struct Spinlock *lock);

PUBLIC void SpinLock(struct Spinlock *lock);
PUBLIC void SpinUnlock(struct Spinlock *lock);

PUBLIC enum InterruptStatus SpinLockSaveIntrrupt(struct Spinlock *lock);
PUBLIC void SpinUnlockRestoreInterrupt(struct Spinlock *lock, enum InterruptStatus oldStatus);

PUBLIC void SpinLockIntrrupt(struct Spinlock *lock);
PUBLIC void SpinUnlockInterrupt(struct Spinlock *lock);

#endif   /*_BOOK_SPINLOCK_H*/
