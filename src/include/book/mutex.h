/*
 * file:		include/book/mutex.h
 * auther:		Jason Hu
 * time:		2019/11/16
 * copyright:   (C) 2018-2019 by Book OS developers. All rights reserved.
 */

/* 
互斥锁：
    互斥锁（Mutex）是在原子操作API的基础上实现的信号量行为。互斥锁不能进行递归锁定或解锁，
能用于交互上下文但是不能用于中断上下文，同一时间只能有一个任务持有互斥锁，而且只有这个任务可以对互斥锁进行解锁。
当无法获取锁时，线程进入睡眠等待状态。

    互斥锁是信号量的特例。信号量的初始值表示有多少个任务可以同时访问共享资源，如果初始值为1，表示只有1个任务可以访问，
信号量变成互斥锁（Mutex）。但是互斥锁和信号量又有所区别，互斥锁的加锁和解锁必须在同一线程里对应使用，所以互斥锁只能用于线程的互斥；
信号量可以由一个线程释放，另一个线程得到，所以信号量可以用于线程的同步。

1、互斥锁和信号量比较
    a、互斥锁功能上基本与二元信号量一样，但是互斥锁占用空间比信号量小，运行效率比信号量高。所以，如果要用于线程间的互斥，优先选择互斥锁。

2、互斥锁和自旋锁比较
    a、互斥锁在无法得到资源时，内核线程会进入睡眠阻塞状态，而自旋锁处于忙等待状态。因此，如果资源被占用的时间较长，使用互斥锁较好，
        因为可让CPU调度去做其它进程的工作。

    b、如果被保护资源需要睡眠的话，那么只能使用互斥锁或者信号量，不能使用自旋锁。而互斥锁的效率又比信号量高，所以这时候最佳选择是互斥锁。

    c、中断里面不能使用互斥锁，因为互斥锁在获取不到锁的情况下会进入睡眠，而中断是不能睡眠的。
 */

#ifndef _BOOK_MUTEX_H
#define _BOOK_MUTEX_H


#include <book/config.h>
#include <book/atomic.h>
#include <book/spinlock.h>
#include <book/task.h>

typedef struct Mutex {
    /* 为1表示未上锁，0表示上锁，小于0表示有等待者 */
    Atomic_t count;

    /* 用于对count的运算和判断多步骤进行保护，使得它们是原子操作 */
    Spinlock_t countLock;
    
    /* waitList是多任务(进程/线程)共享的，需要采用自旋锁互斥访问，
    自旋锁会不停循环检查且不会阻塞任务的执行，适合时间短的加锁。
    waitLock就是拿来保护waitList的访问的 */
    Spinlock_t waitLock;

    
    struct List waitList;   /* 等待队列 */

    struct Task *owner;     /* 锁的持有者 */
} Mutex_t;

/* 初始化互斥锁 */
#define MUTEX_INIT(lockname) \
        { .count = ATOMIC_INIT(1) \
        , .countLock = SPIN_LOCK_INIT_UNLOCKED((lockname).countLock) \
        , .waitLock = SPIN_LOCK_INIT_UNLOCKED((lockname).waitLock) \
        , .waitList = LIST_HEAD_INIT((lockname).waitList) \
        , .owner = NULL }

/* 定义一个互斥锁 */
#define DEFINE_MUTEX(lockname) \
        struct Mutex lockname = MUTEX_INIT(lockname)

#define MUTEX_CLEAR_OWNER(lockname) \
        (lockname)->owner = NULL

#define MUTEX_SET_OWNER(lockname) \
        (lockname)->owner = CurrentTask()

PUBLIC void MutexInit(struct Mutex *lock);

PUBLIC void MutexLock(struct Mutex *lock);
PUBLIC void MutexUnlock(struct Mutex *lock);

PUBLIC int MutexTryLock(struct Mutex *lock);
PUBLIC int MutexIsLocked(struct Mutex *lock);

PUBLIC void DumpMutex(struct Mutex *lock);

#endif   /* _BOOK_MUTEX_H */
