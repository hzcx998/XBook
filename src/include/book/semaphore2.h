/*
 * file:		include/book/semaphore2.h
 * auther:		Jason Hu
 * time:		2019/7/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_SEMAPHORE2_H
#define _BOOK_SEMAPHORE2_H

#include <lib/types.h>
#include <lib/const.h>
#include <book/list.h>

/**
 * 这种信号量是二元信号量，也就是说只有0和1两种值
 * 
 */

struct Semaphore2 {
	unsigned char value;	// 二元信号量值
	struct List waiters;	// 在此信号量上等待的进程
};

/**
 * SemaphoreInit - 信号量的初始化
 * sema: 信号量结构体
 * value: 初始值
 */
PRIVATE INLINE void Semaphore2Init(struct Semaphore2 *sema, unsigned char vaule)
{
    sema->value = vaule;
    // 初始化等待队列头
    INIT_LIST_HEAD(&sema->waiters);
}


/**
 * Semaphore2Down - 信号量的down操作
 * sema: 要操作的信号量
 * 
 * 执行down，如果发现锁被占用，自己就会进入信号量的等待并且阻塞
 * 如果锁没有被占用，那么自己就获取锁
 */
PRIVATE INLINE void Semaphore2Down(struct Semaphore2 *sema)
{
    /* 关闭中断保证原子操作 */
    unsigned long flags = InterruptSave();

    struct Task *current = CurrentTask();
    
    /* 如果信号量为0，那么就被其他任务占用了，所以要等待 */
    while (sema->value == 0) {
        // 保证当前进程不再信号量的等待队列中
        ASSERT(!ListFind(&current->list, &sema->waiters));
        
        // 如果找到了就发生错误
        if (ListFind(&current->list, &sema->waiters)) {
            Panic("SemaphoreDown: task has in waiter list!\n");
        }

        // 添加到信号量等待队列最后
        ListAddTail(&current->list, &sema->waiters);
        
        // 阻塞自己
        TaskBlock(TASK_BLOCKED);
    }

    /* 如果value变成1或者被唤醒后，就可以获得锁 */
    
    // 获得锁之后，信号量--，其它任务如果调用此函数就添加到等待队列，并且阻塞
    sema->value--;

    // 此时信号量一定为0
    ASSERT(sema->value == 0);

    /* 恢复之前的状态 */
    InterruptRestore(flags);
}

/**
 * Semaphore2Up - 信号量的up操作
 * sema: 要操作的信号量
 * 
 * 执行up，释放信号量，就是从之前的信号等待队列中获取一个等待的任务
 * 如果队列为空就表示没有任务在等待这个信号量，那么就直接释放信号量
 * 并且返回。如果有等待的，那么，就把它唤醒，并从等待队列中删除，
 * 让他去获取信号量
 */
PRIVATE INLINE void Semaphore2Up(struct Semaphore2 *sema)
{
    /* 关闭中断保证原子操作 */
    unsigned long flags = InterruptSave();

    // 执行up操作是，信号量必须为0，也就是锁已经被占用
    ASSERT(sema->value == 0);

    // 检测等待队列不为空，因为要从里面获取一个任务，并唤醒它
    if (!ListEmpty(&sema->waiters)) {
        // 获取等待队列中的第一个任务
        struct Task *task = ListFirstOwner(&sema->waiters, struct Task, list);

        // 把任务从等待队列中删除
        ListDel(&task->list);

        // 解除阻塞
        TaskUnblock(task);
    }

    // 释放信号量
    sema->value++;

    // 释放完后，信号量一定是1
    ASSERT(sema->value == 1);

    /* 恢复之前的状态 */
    InterruptRestore(flags);
    
}


#endif   /*_BOOK_SEMAPHORE2_H*/
