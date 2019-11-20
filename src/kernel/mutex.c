/*
 * file:		kernel/mutex.c
 * auther:	    Jason Hu
 * time:		2019/11/16
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/debug.h>
#include <book/mutex.h>
#include <book/task.h>
#include <book/schedule.h>
#include <book/waitqueue.h>

/**
 * MutexInit - 初始化自旋锁
 * @lock: 锁对象
 */
PUBLIC void MutexInit(struct Mutex *lock)
{
    /* 默认是1，表示没有空闲 */
    AtomicSet(&lock->count, 1);
    /* 初始化维护等待队列的自旋锁 */
    SpinLockInit(&lock->waitLock);
    /* 初始化等待队列 */
    INIT_LIST_HEAD(&lock->waitList);

    /* 清除锁的持有者 */
    MUTEX_CLEAR_OWNER(lock);
}

PUBLIC void DumpMutex(struct Mutex *lock)
{
    printk(PART_TIP "----Mutext----\n");
    printk(PART_TIP "self:%x count:%d \n", lock, AtomicGet(&lock->count));
}

/**
 * __MutexLock - 自旋锁加锁
 * @lock: 锁对象
 */
PUBLIC int __MutexLock(struct Mutex *lock)
{
    /*
    获取锁把count从1变成0
    */
    
    /* 先把锁值减1，如果不为负，就说明获取成功，就直接返回 */
    AtomicDec(&lock->count);
    if (AtomicGet(&lock->count) >= 0)
        return 0;     /* 获取成功，返回 */ 

    /* 锁已经被其它任务获取了，自己需要等待锁被释放 */
    
    /* 获取当前的任务 */
    struct Task *current =  CurrentTask();

    /* 等待队列 */
    struct WaitQueue waiter;

    /* 对该锁上的等待队列加自旋锁，防止多个CPU的情况。 */
    SpinLock(&lock->waitLock);

    /* 加入等待队列,FIFO */
    ListAddTail(&waiter.waitList, &lock->waitList);
    waiter.task = current;

    int oldValue;
   
    /* 如果还是没有获得锁，循环获取锁 */
    do {
        /* 用原子操作赋值，lock->count=-1, 保证该操作在一个cpu上是原子的 */
        oldValue = ATOMIC_XCHG(&lock->count, -1);
        //printk("\nold value:%d\n", oldValue);

        /* 如果lock->count之前的值为1，说明是可以获取锁的 */
        if (oldValue == 1)
            break;      /* 获得了锁，跳出循环 */

        /* 如果是可以接受信号的，就去处理收到的信号，并退出循环 */

        /* 设置成阻塞状态 */
        SET_TASK_STATUS(current, TASK_BLOCKED);

        /* 如果还不能获取所，则将自旋锁解除，当从schedule返回时再次获取自旋锁，
        重复如上操作。*/
        SpinUnlock(&lock->waitLock);

        /* 让出cpu执行，既不是休眠等待唤醒，也不是直接循环等待，介于2者之间。耗时：Spinlock < Mutex < Semaphore */
        Schedule();

        /* 对该锁上的等待队列加自旋锁，防止多个CPU的情况。 */
        SpinLock(&lock->waitLock);

        //printk("wakeup %s\n", current->name);
    } while (1);

    /* 获取了锁 */

    /* 将任务从等待队列中移除 */
    ListDel(&waiter.waitList);
    waiter.task = NULL;

    /* 如果等待队列为空将lock->count置为0，就是说只有当前任务获得了锁 */
    if (ListEmpty(&lock->waitList)) {
        AtomicSet(&lock->count, 0);
    }

    /* 解锁等待队列自旋锁。 */
    SpinUnlock(&lock->waitLock);
    return 0;
}


/**
 * MutexLock - 自旋锁加锁
 * @lock: 锁对象
 */
PUBLIC void MutexLock(struct Mutex *lock)
{
    /*
    获取锁把count从1变成0
    */
    __MutexLock(lock);

    /* 设置锁的持有者为当前任务 */
    MUTEX_SET_OWNER(lock);
}

/**
 * __MutexUnlock - 自旋锁解锁
 * @lock: 锁对象
 */
PUBLIC int __MutexUnlock(struct Mutex *lock)
{
    /* 对引用计数器实行加1操作，如果加后小于等于0，
    说明该等待队列上还有任务需要获取锁 */
    AtomicInc(&lock->count);
    if (AtomicGet(&lock->count) == 1)
        return 0;     /* 没有任务请求了，返回 */ 
    
    /* 现在count != 0, 等待队列上还有任务要请求 */

    /* 对该锁上的等待队列加自旋锁，防止多个CPU的情况。 */
    SpinLock(&lock->waitLock);

    /* 修复计数器的值，解锁的时候，只能允许计数器的值为1 */
    if (AtomicGet(&lock->count) > 1) {
        AtomicSet(&lock->count, 1);
    }
    
    /* 先看看等待队列是不是为空了，如果已经为空，
    不需要做任何处理，否则将该等待队列上面的队首进程唤醒 */
    if (!ListEmpty(&lock->waitList)) {
        /* 获取第一个等待者 */
        struct WaitQueue *waiter = ListFirstOwner(&lock->waitList, struct WaitQueue, waitList);

        /* 唤醒任务 */
        TaskWakeUp(waiter->task);
    }

    /* 解锁等待队列自旋锁。 */
    SpinUnlock(&lock->waitLock);

    return 0;
}

/**
 * MutexUnlock - 自旋锁解锁
 * @lock: 锁对象
 */
PUBLIC void MutexUnlock(struct Mutex *lock)
{
    /*
    释放锁把count从0变成1
    */
    __MutexUnlock(lock);

    /* 清空互斥锁owner(此处不需要加自旋锁，不存在读写冲突问题，
    没有获取到锁的任务不会读写owner，没有获取到互斥的任务更不会释放互斥锁) */
    MUTEX_CLEAR_OWNER(lock);
}

/**
 * MutexTryLock - 尝试自旋锁加锁
 * @lock: 锁对象
 * 
 * 非阻塞式获取锁
 * 如果锁已经被使用，就返回一个0值，不会等待锁释放。
 * 如果成功获得了锁，就返回1
 */
PUBLIC int MutexTryLock(struct Mutex *lock)
{
    /* 先把锁值减1，如果不为负，就说明获取成功 */
    AtomicDec(&lock->count);
    if (AtomicGet(&lock->count) >= 0)
        return 1;     /* 获取成功，返回1 */ 
    
    /* 锁已经被其它任务获取了，自己不用等待锁被释放，返回0 */
    return 0;
}

/**
 * MutexIsLocked - 检测锁是否被占用
 * @lock: 锁对象
 * 
 * 如果锁已经被使用，就返回1
 * 不然就返回0
 */
PUBLIC int MutexIsLocked(struct Mutex *lock)
{
    /* 如果锁是<=0，就说明已经被占用 */
    if (AtomicGet(&lock->count) <= 0) {
        return 1;
    }
    /* 没有被占用 */
    return 0;
}