/*
 * file:		kernel/ioqueue.c
 * auther:		Jason Hu
 * time:		2019/8/21
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/ioqueue.h>
#include <share/string.h>
#include <book/mmu.h>
#include <book/debug.h>

/**
 * CreateIoQueue - 动态创建一个io队列
 */
PUBLIC struct IoQueue *CreateIoQueue()
{
	/* 直接分配一块内存并返回 */
	return (struct IoQueue *)kmalloc(IO_QUEUE_SIZE, GFP_KERNEL);
}

/**
 * IoQueueInit - io队列的初始化
 * @ioQueue: io队列
 */
PUBLIC int IoQueueInit(struct IoQueue *ioQueue)
{
	//初始化队列锁
	SynclockInit(&ioQueue->lock);
	//初始化缓冲区
	ioQueue->buf = (unsigned int *)kmalloc(IO_QUEUE_BUF_LEN*4, GFP_KERNEL);
	if (ioQueue->buf == NULL) {
		return -1;
	}
	memset(ioQueue->buf, 0, IO_QUEUE_BUF_LEN*4);

	/* 把头指针和尾指针都指向buf */
	ioQueue->head = ioQueue->tail = ioQueue->buf;
	
	/* 现在还没有数据，所以是0 */
	ioQueue->size = 0;
	
	/* 还没有生产者和消费者 */
	ioQueue->producer = ioQueue->consumer = NULL;
	return 0;
}


PRIVATE void IoQueueWait(struct Task **waiter)
{
	ASSERT(*waiter == NULL && waiter != NULL);
	*waiter = CurrentTask();
	TaskBlock(TASK_BLOCKED);
}

PRIVATE void IoQueueWakeUp(struct Task **waiter)
{
	ASSERT(*waiter != NULL);
	TaskUnblock(*waiter);
	*waiter = NULL;
}

/**
 * IoQueuePut - 往io队列中放入一个数据
 * @ioQueue: io队列
 * @data: 数据
 */
PUBLIC void IoQueuePut(struct IoQueue *ioQueue, unsigned int data)
{
	/*如果队列已经满了，就不能放入数据*/
	while (IoQueueFull(ioQueue)) {
		/* 获取锁 */
		SyncLock(&ioQueue->lock);

		/* 进入等待队列 */
		IoQueueWait(&ioQueue->producer);
		/* 释放锁 */
		SyncUnlock(&ioQueue->lock);
	}
	
	//如果最大了就直接覆盖原有信息
	*ioQueue->head = data;
	
	ioQueue->head++;	//改变指针位置
	ioQueue->size++;	//数据数量增加
	//修复越界
	if(ioQueue->head >= ioQueue->buf + IO_QUEUE_BUF_LEN){
		ioQueue->head = ioQueue->buf;
	}

	/* 唤醒消费者*/
	if(ioQueue->consumer != NULL){
		IoQueueWakeUp(&ioQueue->consumer);
	}
}

/**
 * IoQueueGet - 从io队列中获取一个数据
 * @ioQueue: io队列
 */
PUBLIC unsigned int IoQueueGet(struct IoQueue *ioQueue)
{
	/*如果队列时空的，就一直等待，知道有数据产生*/
	while (IoQueueEmpty(ioQueue)) {
		/* 获取锁 */
		SyncLock(&ioQueue->lock);
		/* 消费者进入等待状态 */
		IoQueueWait(&ioQueue->consumer);
		/* 释放锁 */
		SyncUnlock(&ioQueue->lock);
	}

	/* 从尾部获取一个数据 */
	unsigned int data = *ioQueue->tail;
	
	//改变指针位置
	ioQueue->tail++;	
	
	//数据数量减少
	ioQueue->size--;

	/* 如果到达了最后面，就跳到最前面，形成一个环 */
	if(ioQueue->tail >= ioQueue->buf + IO_QUEUE_BUF_LEN){
		ioQueue->tail = ioQueue->buf;
	}

	/* 如果有生产者，就唤醒生产者 */
	if(ioQueue->producer != NULL){
		IoQueueWakeUp(&ioQueue->producer);
	}
	return data;
}
