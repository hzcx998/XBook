/*
 * file:		kernel/task/spinlock.c
 * auther:	    Jason Hu
 * time:		2019/11/15
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/debug.h>
#include <book/spinlock.h>

/**
 * SpinLockInit - 初始化自旋锁
 * @lock: 锁对象
 */
PUBLIC void SpinLockInit(struct Spinlock *lock)
{
    /* 初始化为0，表示还没有被使用 */
    AtomicSet(&lock->lock, 0);
}

/**
 * SpinLock - 自旋锁加锁
 * @lock: 锁对象
 */
PUBLIC void SpinLock(struct Spinlock *lock)
{
    /* 对于多处理器，应该先禁止内核抢占，由于目前是单处理器，就可以不用写抢占 */

    /* 锁被使用的标志 */
    char used;
    
    do {
        /* 默认是没有被使用 */
        used = 0;

        /* 如果锁已经被其它进程或线程占用，自己就要循环 */
        while (AtomicGet(&lock->lock) > 0) {
            used = 1;
        }

        #ifdef SPINLOCK_DEBUG
        /* 如果有调试的话，那么就把占用了的 */
        if (used) {
            printk("<Lock>");
        }
        #endif
        /*  
        1.used = 1：
            锁被其它占用，并且已经释放了，自己可以尝试获取 
        2.used = 0：
            自己获取锁之后，就可以直接退出
        */

    } while (used);    /* 如果锁被使用了，就要重复检测 */

    /* 执行上锁操作，获取锁 */
    AtomicInc(&lock->lock);
}

/**
 * SpinUnlock - 自旋锁解锁
 * @lock: 锁对象
 */
PUBLIC void SpinUnlock(struct Spinlock *lock)
{
    /* 执行解锁操作，释放锁，其实就是把锁恢复初始值 */
    AtomicSet(&lock->lock, 0);

    /* 对于多处理器，应该打开内核抢占，由于目前是单处理器，就可以不用打开内核抢占 */
}

/**
 * SpinLockIrqSave - 自旋锁加锁并保存中断状态
 * @lock: 锁对象
 * 
 * 会关闭中断
 * 返回进入前的中断状态
 */
PUBLIC unsigned long SpinLockIrqSave(struct Spinlock *lock)
{
    /* 获取中断状态并关闭中断 */
    unsigned long eflags = LoadEflags();
    DisableInterrupt();

    /* 自旋锁加锁 */
    SpinLock(lock);

    /* 返回之前的状态 */
    return eflags;
}

/**
 * SpinUnlockIrqSave - 自旋锁解锁并恢复中断状态
 * @lock: 锁对象
 * @eflags: 之前的状态
 * 恢复进入自旋锁之前的中断状态
 */
PUBLIC void SpinUnlockIrqSave(struct Spinlock *lock, unsigned long eflags)
{
    /* 自旋锁解锁 */
    SpinUnlock(lock);
    
    /* 中断恢复之前的状态 */
    StoreEflags(eflags);
}

/**
 * SpinLockIntrrupt - 自旋锁加锁并关闭中断
 * @lock: 锁对象
 * 
 * 会关闭中断
 * 返回进入前的中断状态
 */
PUBLIC void SpinLockIntrrupt(struct Spinlock *lock)
{
    /* 关闭中断 */
    DisableInterrupt();

    /* 自旋锁加锁 */
    SpinLock(lock);
}

/**
 * SpinUnlockInterrupt - 自旋锁解锁并打开中断
 * @lock: 锁对象
 * 
 * 恢复进入自旋锁之前的中断状态
 */
PUBLIC void SpinUnlockInterrupt(struct Spinlock *lock)
{
    /* 自旋锁解锁 */
    SpinUnlock(lock);
    
   /* 打开中断 */
    EnableInterrupt();
}


/**
 * SpinTryLock - 尝试自旋锁加锁
 * @lock: 锁对象
 * 
 * 
 * 非阻塞式获取锁
 * 如果锁已经被使用，就返回一个非0值，不会自旋等待锁释放。
 * 如果成功获得了锁，就返回0
 */
PUBLIC int SpinTryLock(struct Spinlock *lock)
{
    /* 对于多处理器，应该先禁止内核抢占，由于目前是单处理器，就可以不用写抢占 */

    /* 如果锁已经被其它进程或线程占用，返回非0，不等待 */
    if (AtomicGet(&lock->lock) > 0) {
        return 1;
    }
    
    /* 执行上锁操作，获取锁 */
    AtomicInc(&lock->lock);

    /* 对于多处理器，应该打开内核抢占，由于目前是单处理器，就可以不用打开内核抢占 */

    return 0;
}

/**
 * SpinIsLocked - 检测锁是否被占用
 * @lock: 锁对象
 * 
 * 如果锁已经被使用，就返回1
 * 不然就返回0
 */
PUBLIC int SpinIsLocked(struct Spinlock *lock)
{
    /* 如果锁是非0，就说明已经被占用 */
    if (AtomicGet(&lock->lock) > 0) {
        return 1;
    }
    /* 没有被占用 */
    return 0;
}