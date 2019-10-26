/*
 * file:	    kernel/block.c
 * auther:	    Jason Hu
 * time:	    2019/10/7
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/debug.h>
#include <share/string.h>
#include <book/block.h>
#include <book/device.h>
#include <book/task.h>
#include <book/bitops.h>
#include <driver/clock.h>

/* 完成的请求的链表 */
LIST_HEAD(doneRequestListHead);

/**
 * GetRequestFromDoneList - 从完成请求队列中获取一个请求
 * 
 * 当一个请求被使用之后，就会把它添加到完成请求队列。这样可以将它重复利用
 */
PRIVATE struct Request *GetRequestFromDoneList()
{
    struct Request *req;

    if (ListEmpty(&doneRequestListHead)) {
        return NULL;
    }
    /* 剩余则摘取一个并返回 */
    req = ListFirstOwner(&doneRequestListHead, struct Request, queueList);
    
    ListDel(&req->queueList);

    return req;
}

/**
 * SwitchRequestList - 切换请求链表指向
 * @queue: 请求队列
 * 
 *    在拿取一个请求的时候，如果当前的请求队列为空，那么就切换以下请求队列
 * 去查看另外一个请求队列的情况。
 *    在我的电梯调度算法中，使用2个请求队列，一个up请求队列（upRequestList），
 * 一个down请求队列（upRequestList）。
 *    分别用来表示要操作的块号的排序方向，up是递增，down是递减的队列。
 * 分别用来保存向上移动和像下移动的块号。
 * 他们和当前请求（currentRequest）紧密相关。
 */
PRIVATE void SwitchRequestList(struct RequestQueue *queue)
{

    /* 切换请求队列指向 */
    if (queue->requestList == &queue->upRequestList) {
        queue->requestList = &queue->downRequestList;
    } else {
        queue->requestList = &queue->upRequestList;
    }
}

/**
 * AddRequest - 添加一个请求
 * @queue: 请求队列
 * @request: 请求
 * 
 * 把请求添加到请求队列
 */
PRIVATE void AddRequest(struct RequestQueue *queue, struct Request *req)
{
    struct Request *tmp;

    /* 准备添加到请求队列的时候，不允许产生其它进程进行访问 */
    enum InterruptStatus oldStatus = InterruptDisable();

    /* 记录请求所在的队列 */
    req->queue = queue;

    /* 把请求插入合适的位置 */
    ElevatorIoSchedule(queue, req);

    
    if (req->bh) {
        /* 把缓冲区的脏位去掉，因为将会读取全新的数据 */
        req->bh->dirty = 0;
    }
    
    /* 如果请求队列没有请求，就立即执行当前请求 */
    tmp = queue->currentRequest;

    InterruptSetStatus(oldStatus); 

    if (tmp == NULL) {
        /* 更新当前请求 */
        queue->currentRequest = req;

        BlockStartRequeue(req);
        return;
    }
    
    /* 如果需要等待请求队列，那么自己就先休眠 */
    TaskSleepOn(CurrentTask());
}

/**
 * BlockInitQueue - 初始化块请求队列
 * @callback: 请求的回调函数
 * @queuedata: 请求的私有数据
 */
PUBLIC struct RequestQueue *BlockInitQueue(requestFunction_t callback, void *queuedata)
{
    struct RequestQueue *rq = kmalloc(sizeof(struct RequestQueue), GFP_KERNEL);
    if (rq == NULL) {
        return NULL;
    }

    INIT_LIST_HEAD(&rq->upRequestList);
    INIT_LIST_HEAD(&rq->downRequestList);

    /* 默认指向上请求队列 */
    rq->requestList = &rq->upRequestList;

    rq->currentRequest = NULL;
    rq->queuedata = queuedata;

    rq->requestFunction = callback;

    return rq;
}

/**
 * MakeRequest - 生成一个请求
 * @major: 主设备号
 * @rw: 读/写操作
 * @bh: 缓冲头
 * 
 * 通过参数生成一个请求
 */
PUBLIC void MakeRequest(int major, int rw, struct BufferHead *bh)
{

    if (rw != BLOCK_READ && rw != BLOCK_WRITE)
        Panic("Bad block dev command, must be R/W");  

    LockBuffer(bh);

    /* 如果是写，并且没有脏数据，就直接返回。
    如果是读，并且数据有效，就直接返回 */
    if ((rw == BLOCK_WRITE && !bh->dirty) || 
    (rw == BLOCK_READ && bh->uptodate)) {
        UnlockBuffer(bh);
        return;
    }

    /* 初始化请求信息 */
    struct Request *req;
    
    req = GetRequestFromDoneList();
    if (req == NULL) 
        req = kmalloc(SIZEOF_REQUEST, GFP_KERNEL);
    
    if (req == NULL) {
        UnlockBuffer(bh);
        return;
    }
    memset(req, 0, SIZEOF_REQUEST);

    /* 通过major获取对应的设备的disk */
    struct BlockDevice *dev = GetBlockDeviceByDevno(bh->devno);
    if (dev == NULL) {
        UnlockBuffer(bh);
        return;
    }

    req->buffer = bh->data;
    req->devno = bh->devno;
    /* 根据块设备的块大小来判断请求需要的扇区数 */
    req->count = dev->blockSize / SECTOR_SIZE;
    req->cmd = rw;
    req->lba = bh->lba;
    req->errors = 0;
    req->bh = bh;
    req->waiter = CurrentTask();    /* 当前任务就是等待者 */

    /* 把请求添加到磁盘的请求队列 */
    AddRequest(dev->disk->requestQueue, req);
}


/**
 * BlockStartRequeue - 执行一个请求
 * @request: 要执行的请求
 * 
 * 执行请求，其实就是调用设备驱动中的请求函数
 */
PUBLIC void BlockStartRequeue(struct Request *request)
{
    if (request->queue == NULL)
        return;
    /* 执行请求 */
    request->queue->requestFunction(request->queue);
}

/**
 * BlockFetchRequest - 从请求队列摘取一个请求
 * @queue: 请求队列
 * 
 * 如果请求队列中还有请求，就摘取，不然就返回空
*/
PUBLIC struct Request *BlockFetchRequest(struct RequestQueue *queue)
{
    if (queue == NULL)
        return NULL;
    
    /* 获取请求过程中，不允许产生中断 */
    enum InterruptStatus oldStatus = InterruptDisable();

    /* 检测是否为空 */
    if (ListEmpty(queue->requestList)) {
        /* 切换成；另外一个请求队列再检测 */
        SwitchRequestList(queue);

        /* 两个请求队列都位空，那么就没有请求了 */
        if (ListEmpty(queue->requestList)) {
            goto ToEnd;
        }
    }
    
    /* 摘取一个请求 */
    struct Request *request = ListFirstOwner(queue->requestList,
        struct Request, queueList);
    if (request == NULL) {
        goto ToEnd;
    }

    /* 脱离链表 */
    ListDel(&request->queueList);

    /* 更新当前请求指针 */
    queue->currentRequest = request;

    InterruptSetStatus(oldStatus);

    return request;
ToEnd:
    InterruptSetStatus(oldStatus);
    return NULL;
}

/**
 * BlockEndRequest - 结束一个请求
 * @request: 要结束的请求
 * @errors: 在请求中发生的错误数
 * 
 * 结束一个请求，设置请求对应的缓冲头和任务的状态并把请求回收到
 * 完成请求队列，已备重复利用
 */
PUBLIC void BlockEndRequest(struct Request *request, int errors)
{
    if (request == NULL)
        return;

    /* 结束请求过程中，不允许产生中断 */
    enum InterruptStatus oldStatus = InterruptDisable();

    /* 记录请求错误数，可以根据请求的错误数做一些设定 */
    request->errors += errors;
    
    if (request->bh) {
        /* 把对应得bh设置为更新 */
        request->bh->uptodate = 1;

        /* 解除阻塞 */
        UnlockBuffer(request->bh);

        /* 释放对它的占用 */
        PutBH(request->bh);

        if (!request->bh->uptodate) {
            printk("device %d I/O error!\n", request->devno);
        }
    }

    /* 唤醒等待请求中的任务 */
    TaskWakeUp(request->waiter);

    /* 结束请求的时候设置当前请求为空 */
    request->queue->currentRequest = NULL;
    
    /* 把request添加到完成队列，以便重复利用 */
    ListAdd(&request->queueList, &doneRequestListHead);

    InterruptSetStatus(oldStatus);
}

/**
 * BlockCleanUpQueue - 清除块设备对应的队列
 * @queue: 队列
 * 
 * 其实就是释放队列占用的内存，这在设备驱动中创建
*/
PUBLIC void BlockCleanUpQueue(struct RequestQueue *queue)
{
    if (queue != NULL)
        kfree(queue);
}

PUBLIC void DumpRequest(struct Request *request)
{
    printk(PART_TIP "----Request----\n");
    printk(PART_TIP "queue:%x disk:%x bh:%x data:%x \n",
        request->queue, request->disk, request->bh, request->buffer);
    printk(PART_TIP "devno:%x cmd:%d errors:%d lba:%d count:%d\n", 
        request->devno, request->cmd, request->errors, request->lba, request->count);
}

PUBLIC void DumpRequestQueue(struct RequestQueue *queue)
{
    printk(PART_TIP "----Request Queue----\n");
    printk(PART_TIP "function:%x current:%x data:%x \n",
        queue->requestFunction, queue->currentRequest, queue->queuedata);
}
