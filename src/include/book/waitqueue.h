/*
 * file:		include/book/waitqueue.h
 * auther:		Jason Hu
 * time:		2019/8/21
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_WAITQUEUE_H
#define _BOOK_WAITQUEUE_H

#include <share/types.h>
#include <book/list.h>
#include <book/arch.h>
#include <book/task.h>
#include <book/debug.h>

struct WaitQueue {
	struct List waitList;	// 记录所有被挂起的进程（等待中）的链表
	struct Task *task;		// 当前被挂起的进程
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
 * WaitQueueAdd - 把进程添加到等待队列中
 * @waitQueue: 等待队列
 * @task: 要添加的任务
 */
PRIVATE INLINE void WaitQueueAdd(struct WaitQueue *waitQueue, struct Task *task)
{
	/* 添加到队列时，需要关闭中断 */
	enum InterruptStatus oldStatus = InterruptDisable();

	/* 确保任务不在等待队列中 */
	ASSERT(!ListFind(&task->list, &waitQueue->waitList));
	
	/* 添加到等待队列中，添加到最后 */
	ListAddTail(&task->list, &waitQueue->waitList);

	InterruptSetStatus(oldStatus);
}

/**
 * WaitQueueRemove - 把进程从等待队列中移除
 * @waitQueue: 等待队列
 * @task: 要移除的任务
 */
PRIVATE INLINE void WaitQueueRemove(struct WaitQueue *waitQueue, struct Task *task)
{
	/* 添加到队列时，需要关闭中断 */
	enum InterruptStatus oldStatus = InterruptDisable();

	struct Task *target, *next;
	/* 在队列中寻找任务，找到后就把任务从队列中删除 */
	ListForEachOwnerSafe(target, next, &waitQueue->waitList, list) {
		if (target == task) {
			/* 把任务从等待队列中删除 */
			ListDelInit(&target->list);
			break;
		}
	}
	
	InterruptSetStatus(oldStatus);
}
 
/**
 * WaitQueueWakeUp - 唤醒等待队列中的一个任务
 * @waitQueue: 等待队列
 */
PRIVATE INLINE void WaitQueueWakeUp(struct WaitQueue *waitQueue)
{
	/* 添加到队列时，需要关闭中断 */
	enum InterruptStatus oldStatus = InterruptDisable();

	/* 不是空队列就获取第一个等待者 */
	if (!ListEmpty(&waitQueue->waitList)) {
		/* 获取任务 */
		struct Task *task = ListFirstOwner(&waitQueue->waitList, struct Task, list);
		
		/* 从当前队列删除 */
		ListDel(&task->list);

		/* 唤醒任务 */
		TaskWakeUp(task);
	}
	InterruptSetStatus(oldStatus);
}

#endif   /*_BOOK_WAITQUEUE_H*/
