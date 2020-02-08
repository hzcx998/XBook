/*
 * file:		include/book/spinlock.h
 * auther:		Jason Hu
 * time:		2019/11/15
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_SPINLOCK_H
#define _BOOK_SPINLOCK_H

/*
自旋锁：
    在多核处理器系统中,系统不允许在不同的CPU上运行的内核控制路径同时访问某些内核数据结构,
在这种情况下如果修改数据结构所需的时间比较短，那么信号量（参考信号量）是低效的。
这是系统会使用自旋锁，当一个进程发现锁被另一个进程锁着时，他就不停”旋转”，执行一个紧凑的循环指令直到锁被打开。

    但是这种自旋锁在单核处理器下是无效的。当内核控制路径试图访问一个上锁的数据结构，他就开始无休止的循环。因此，
内核控制路径可能因为正在修改受保护的数据结构而没有机会继续执行，也没有机会释放这个自旋锁。最后的结果可能是系统挂起。

（1） 一个被争用的自旋锁使得请求它的线程在等待锁重新可用时自旋，会特别浪费处理器时间。
    所以自旋锁不应该被长时间持有。因此，自旋锁应该使用在：短时间内进行轻量级加锁。

（2）还可以采取另外的方式来处理对锁的争用：让请求线程睡眠，直到锁重新可用时再唤醒它这样处理器不必循环等待，可以执行其他任务。
    但是让请求线程睡眠的处理也会带来一定开销：会有两次上下文切换，被阻塞的线程要换出和换入所以，
    自旋持有锁的时间最好小于完成两次上下文e月刊的耗时，也就是让持有自旋锁的时间尽可能短。
    （在抢占式内核中，的锁持有等价于系统-的调度等待时间），信号量可以在发生争用时，等待的线程能投入睡眠，而不是旋转。

（3）在单处理机器上，自旋锁是无意义的。因为在编译时不会加入自旋锁，仅仅被当作一个设置内核抢占机制是否被启用的开关。
    如果禁止内核抢占，那么在编译时自旋锁会被完全剔除出内核。

（4）自旋锁是不可递归的。如果试图得到一个你正在持有的锁，你必须去自旋，等待你自己释放这个锁。
    但这时你处于自旋忙等待中，所以永远不会释放锁，就会造成死锁现象。

（5）在中断处理程序中，获取锁之前一定要先禁止本地中断（当前处理器的中断），否则，中断程序就会打断正持有锁的内核代码，
    有可能会试图去争用这个已经被持有的自旋锁。这样就会造成双重请求死锁（中断处理程序会自旋，等待该锁重新可用，
    但锁的持有者在这个处理程序执行完之前是不可能运行的）

（6）锁真正保护的是数据（共享数据），而不是代码。对于BLK（大内核锁）保护的是代码。
*/

#include <book/config.h>
#include <book/atomic.h>

//#define SPINLOCK_DEBUG

typedef struct Spinlock {
    Atomic_t lock; /* 锁变量是原子，为0表示空闲，大于等于0表示已经被使用 */
} Spinlock_t;

/* 初始化未上锁的自旋锁 */
#define SPIN_LOCK_INIT_UNLOCKED(lockname) \
        { .lock = ATOMIC_INIT(0) }

/* 初始化上锁的自旋锁 */
#define SPIN_LOCK_INIT_LOCKED(lockname) \
        { .lock = ATOMIC_INIT(1) }

/* 初始化自旋锁，默认是未上锁的自旋锁 */
#define SPIN_LOCK_INIT(lockname) \
        SPIN_LOCK_INIT_UNLOCKED(lockname) 

PUBLIC void SpinLockInit(struct Spinlock *lock);

PUBLIC void SpinLock(struct Spinlock *lock);
PUBLIC void SpinUnlock(struct Spinlock *lock);

PUBLIC enum InterruptStatus SpinLockSaveIntrrupt(struct Spinlock *lock);
PUBLIC void SpinUnlockRestoreInterrupt(struct Spinlock *lock, enum InterruptStatus oldStatus);

PUBLIC void SpinLockIntrrupt(struct Spinlock *lock);
PUBLIC void SpinUnlockInterrupt(struct Spinlock *lock);

PUBLIC int SpinTryLock(struct Spinlock *lock);
PUBLIC int SpinIsLocked(struct Spinlock *lock);


PUBLIC unsigned long SpinLockIrqSave(struct Spinlock *lock);
PUBLIC void SpinUnlockIrqSave(struct Spinlock *lock, unsigned long eflags);



#endif   /*_BOOK_SPINLOCK_H*/
