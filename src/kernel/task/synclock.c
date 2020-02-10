/*
 * file:		kernel/task/synclock.c
 * auther:	    Jason Hu
 * time:		2019/7/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/schedule.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/task.h>
#include <book/synclock.h>
#include <lib/string.h>

/**
 * SynclockInit - 初始化同步锁
 * @lock: 锁对象
 */
PUBLIC void SynclockInit(struct Synclock *lock)
{
    lock->holder = NULL;        // 最开始没有持有者
    lock->holderReaptCount = 0; // 一次都没有重复
    
    // 初始信号量为1，表示可以被使用一次
    SemaphoreInit(&lock->semaphore, 1);  
}

/**
 * SyncLock - 获取同步锁
 * @lock: 锁对象
 */
PUBLIC void SyncLock(struct Synclock *lock)
{
    // 自己没有锁才获取信号量
    if (lock->holder != CurrentTask()) {
        //printk("GET ");
        /* 获取信号量，如果信号量已经被其它任务占用，那么自己就等待
        直到信号量被其它任务释放，自己才可以获取*/
        #ifdef CONFIG_SEMAPHORE_M
        SemaphoreDown(&lock->semaphore);
        #endif
        
        #ifdef CONFIG_SEMAPHORE_B
        Semaphore2Down(&lock->semaphore);
        #endif
        // 当自己成功获取信号量后才把锁的持有者设置成当前任务
        lock->holder = CurrentTask();

        // 有新的任务第一次获取锁，说明锁还没有被任务重复获取
        ASSERT(lock->holderReaptCount == 0);

        // 现在被新的任务获取了，所以现在有1次获取
        lock->holderReaptCount = 1;
    } else {
        /*
        自己已经获取锁，在这里又来获取，所以就直接增加重复次数，
        而不执行获取信号量的操作
         */ 
        lock->holderReaptCount++;
        
    }   
}

/**
 * SyncUnlock - 释放同步锁
 * @lock: 锁对象
 */
PUBLIC void SyncUnlock(struct Synclock *lock)
{
    // 释放的时候自己必须持有锁
    ASSERT(lock->holder == CurrentTask());

    // 如果自己获取多次，那么只有等次数为1时才会真正释放信号量
    if (lock->holderReaptCount > 1) {

        // 减少重复次数并返回
        lock->holderReaptCount--;
        return;
    }

    // 到这儿的话，就可以真正释放信号量了，相当于到达最外层的锁释放
    ASSERT(lock->holderReaptCount == 1);
    //printk("PUT ");
        
    lock->holder = NULL;                // 没有锁的持有者，锁处于未持有状态
    lock->holderReaptCount = 0;         // 释放后没有任务持有锁，重复次数为0
    #ifdef CONFIG_SEMAPHORE_M
    SemaphoreUp(&lock->semaphore);      // 执行信号量up操作，释放信号量
    #endif
    
    #ifdef CONFIG_SEMAPHORE_B
    Semaphore2Up(&lock->semaphore);      // 执行信号量up操作，释放信号量
    #endif
}
