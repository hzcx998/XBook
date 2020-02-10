/*
 * file:		include/book/ioqueue.h
 * auther:		Jason Hu
 * time:		2019/8/21
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_IOQUEQUE_H
#define	_BOOK_IOQUEQUE_H

#include <lib/types.h>
#include <lib/stdint.h>
#include <book/synclock.h>
#include <book/task.h>


#define IQ_QUEUE_IDLE 0
#define IQ_QUEUE_MOVE 1

/* 数据因子大小 */
#define IQ_FACTOR_8     1
#define IQ_FACTOR_16    2
#define IQ_FACTOR_32    4
#define IQ_FACTOR_64    8

/* 默认的缓冲区大小 */
#define IQ_BUF_DATA_NR      64

/* 缓冲大小 */
#define IQ_BUF_LEN_8    (IQ_BUF_DATA_NR * IQ_FACTOR_8)
#define IQ_BUF_LEN_16   (IQ_BUF_DATA_NR * IQ_FACTOR_16)
#define IQ_BUF_LEN_32   (IQ_BUF_DATA_NR * IQ_FACTOR_32)
#define IQ_BUF_LEN_64   (IQ_BUF_DATA_NR * IQ_FACTOR_64)

/*
生产者消费者模型来实现IoQueue
*/
struct IoQueue {
    struct Synclock lock;
    unsigned char *buf;			    // 缓冲区
    unsigned int buflen;            // 缓冲区大小
    unsigned char *head;			    // 队首,数据往队首处写入
    unsigned char *tail;			    // 队尾,数据从队尾处读出
	size_t size;
    /* 因子大小，表示队列中每一个单元的大小。
    8位，16位，32位，64位 */
    char factor;                    
	struct Task *producer;
	struct Task *consumer;
};

#define IO_QUEUE_SIZE sizeof(struct IoQueue)

#define IO_QUEUE_LENGTH(ioqueue) \
        (ioqueue)->size 


PUBLIC struct IoQueue *CreateIoQueue();
PUBLIC int IoQueueInit(struct IoQueue *ioQueue, 
    unsigned char *buf, unsigned int buflen, char factor);

PUBLIC unsigned long IoQueueGet(struct IoQueue *ioQueue);
PUBLIC void IoQueuePut(struct IoQueue *ioQueue, unsigned long data); 



PRIVATE INLINE bool IoQueueEmpty(struct IoQueue *ioQueue)
{
	return (ioQueue->size == 0) ? 1:0;	//如果大小为0就是空的
}

PRIVATE INLINE bool IoQueueFull(struct IoQueue *ioQueue)
{
	return (ioQueue->size == ioQueue->buflen / ioQueue->factor) ? 1:0;	//如果大小为0就是空的
}

#endif /* _BOOK_IOQUEQUE_H */