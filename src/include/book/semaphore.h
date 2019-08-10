/*
 * file:		include/book/semaphore.h
 * auther:		Jason Hu
 * time:		2019/8/9
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_SEMAPHORE_H
#define _BOOK_SEMAPHORE_H

#include <share/types.h>
#include <share/const.h>
#include <book/list.h>
#include <book/atomic.h>

/**
 * 当前这种信号量的值可以是很多，不仅仅局限于二元（0,1）
 */

struct WaitQueue {
	struct List waitList;	// 记录所有被挂起的进程（等待中）的链表
	struct Task *task;		// 当前被挂起的进程
};

struct Semaphore {
	Atomic_t counter;			// 统计资源的原子变量
	struct WaitQueue waiter;	// 在此信号量上等待的进程
};

/**
 * WaitQueueInit - 等待队列初始化
 * @waitQueue: 等待队列
 * @task: 等待的任务
 */
PRIVATE INLINE void WaitQueueInit(struct WaitQueue *waitQueue, struct Task *task)
{
	/* 初始化队列 */
	INIT_LIST_HEAD(&waitQueue->waitList);
	/* 设置等待队列中的任务 */
	waitQueue->task = task;
}

/**
 * SemaphoreInit - 信号量初始化
 * @sema: 信号量
 * @value: 初始值
 */
PRIVATE INLINE void SemaphoreInit(struct Semaphore *sema, int value)
{
	/* 设置成初始值 */
	AtomicSet(&sema->counter, value);
	/* 初始化等待队列 */
	WaitQueueInit(&sema->waiter, NULL);
}

/**
 * __SemaphoreDown - 执行具体的down操作
 * @sema: 信号量
 */
PRIVATE INLINE void __SemaphoreDown(struct Semaphore *sema)
{
	/* 申请一个等待者 */
	struct WaitQueue waiter;
	struct Task *current = CurrentTask();

	/* 把当前进程设置成一个等待者 */
	WaitQueueInit(&waiter, current);

	/* 状态设置成阻塞 */
	current->status = TASK_BLOCKED;
	
	/* 把自己添加到信号量的等待队列中，等待被唤醒 */
	ListAddTail(&waiter.waitList, &sema->waiter.waitList);

	/* 调度到其它任务 */
	Schedule();
}

/**
 * SemaphoreDown - 信号量down
 * @sema: 信号量
 */
PRIVATE INLINE void SemaphoreDown(struct Semaphore *sema)
{
	
	/* 如果计数器大于0，就说明资源没有被占用 */
	if (AtomicGet(&sema->counter) > 0) {
		/* 计数器减1，说明现在信号量被获取一次了 */
		AtomicDec(&sema->counter);
	} else {
		/* 如果信号量为0，说明不能被获取，那么就把当前进程阻塞 */
		__SemaphoreDown(sema);
	}
}

/**
 * __SemaphoreDown - 执行具体的up操作
 * @sema: 信号量
 */
PRIVATE INLINE void __SemaphoreUp(struct Semaphore *sema)
{
	/* 从等待队列获取一个等待者 */
	struct WaitQueue *waiter = ListFirstOwner(&sema->waiter.waitList, struct WaitQueue, waitList);
	
	/* 让等待者的链表从信号量的链表中脱去 */
	ListDel(&waiter->waitList);

	/* 设置任务为就绪状态 */
	waiter->task->status = TASK_READY;

	/* 先检测不在就绪队列中 */
	ASSERT(!ListFind(&waiter->task->list, &taskReadyList));
	/* 添加到就绪队列 */
	ListAdd(&waiter->task->list, &taskReadyList);
}

/**
 * SemaphoreUp - 信号量up
 * @sema: 信号量
 */
PRIVATE INLINE void SemaphoreUp(struct Semaphore *sema)
{

	/* 如果等待队列为空，说明没有等待的任务，就只释放信号量 */
	if (ListEmpty(&sema->waiter.waitList)) {
		/* 使信号量递增 */
		AtomicInc(&sema->counter);
	} else {
		/* 有等待任务，就唤醒一个 */
		__SemaphoreUp(sema);
	}
}

#endif   /*_BOOK_SEMAPHORE_H*/
