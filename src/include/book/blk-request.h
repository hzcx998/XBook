/*
 * file:		include/book/blk-request.h
 * auther:		Jason Hu
 * time:		2019/10/13
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_BLOCK_REQUEST_H
#define _BOOK_BLOCK_REQUEST_H

#include <share/types.h>
#include <book/list.h>
#include <book/block.h>
#include <book/task.h>

#include <book/blk-buffer.h>

/**
 * 请求是每一次操作的请求 
 */
struct Request {
    struct List queueList;      /* 所有请求在请求队列中的链表 */
    
    struct RequestQueue *queue; /* 所在的请求队列（如果有） */ 
    struct Disk *disk;      /* 请求对应的磁盘 */
    dev_t devno;      // 设备号
    int cmd;        // 命令（读或写）
    int errors;     // 错误数
    sector_t lba;    //逻辑块地址
    unsigned long count;    // 操作的块数
    char *buffer;       // 读写用到的buffer
    struct BufferHead *bh;
    struct Task *waiter;
};
#define SIZEOF_REQUEST sizeof(struct Request)

typedef void (*RequestFunction_t)(struct RequestQueue *);

/**
 * 请求队列是每一个块设备都应该有的
 * 
 */
struct RequestQueue {
    struct List upRequestList;  // 上请求链表头
    struct List downRequestList;  // 上请求链表头
    struct List *requestList;

    RequestFunction_t requestFunction;     // 执行请求函数

    struct Request *currentRequest;    /* 当前的请求 */

    void *queuedata;
};

PUBLIC void DumpRequest(struct Request *request);

/* 初始化请求队列 */
PUBLIC struct RequestQueue *BlockInitQueue(RequestFunction_t callback, void *queuedata);

/* 清除请求队列 */
PUBLIC void BlockCleanUpQueue(struct RequestQueue *request);

PUBLIC void MakeRequest(int major, int rw, struct BufferHead *bh);

/* 提取请求队列 */
PUBLIC struct Request *BlockFetchRequest(struct RequestQueue *queue);

/* 启动请求 */
PUBLIC void BlockStartRequeue(struct Request *request);

/* 报告完成 */
PUBLIC void BlockEndRequest(struct Request *request, int errors);

PUBLIC void DumpRequestQueue(struct RequestQueue *queue);

#endif   /* _BOOK_BLOCK_REQUEST_H */
