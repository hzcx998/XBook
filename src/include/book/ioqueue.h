/*
 * file:		include/book/ioqueue.h
 * auther:		Jason Hu
 * time:		2019/8/21
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_IOQUEQUE_H
#define	_BOOK_IOQUEQUE_H

#include <share/types.h>
#include <share/stdint.h>
#include <book/synclock.h>
#include <book/task.h>

#define IO_QUEUE_BUF_LEN 64

#define IQ_QUEUE_IDLE 0
#define IQ_QUEUE_MOVE 1

/*
生产者消费者模型来实现IoQueue
*/
struct IoQueue {
    struct Synclock lock;
    unsigned int *buf;			    // 缓冲区大小
    unsigned int *head;			    // 队首,数据往队首处写入
    unsigned int *tail;			    // 队尾,数据从队尾处读出
	size_t size;
	struct Task *producer;
	struct Task *consumer;
};

#define IO_QUEUE_SIZE sizeof(struct IoQueue)

PUBLIC struct IoQueue *CreateIoQueue();
PUBLIC int IoQueueInit(struct IoQueue *ioQueue);

PUBLIC unsigned int IoQueueGet(struct IoQueue *ioQueue);
PUBLIC void IoQueuePut(struct IoQueue *ioQueue, unsigned int data); 

PRIVATE INLINE bool IoQueueEmpty(struct IoQueue *ioQueue)
{
	return (ioQueue->size == 0) ? 1:0;	//如果大小为0就是空的
}

PRIVATE INLINE bool IoQueueFull(struct IoQueue *ioQueue)
{
	return (ioQueue->size == IO_QUEUE_BUF_LEN) ? 1:0;	//如果大小为0就是空的
}

#endif /* _BOOK_IOQUEQUE_H */