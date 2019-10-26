/*
 * file:	    kernel/block/elevator.c
 * auther:	    Jason Hu
 * time:	    2019/10/13
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/debug.h>
#include <share/string.h>
#include <book/block.h>

#include <book/blk-request.h>

/**
 * ElevatorInUp - 在上移电梯中进行调度处理
 * @queue: 请求队列
 * @request: 请求
 * 
 * 当当前的请求队列指向上移请求队列时，就调用该函数进行I/O调度处理
 */
PRIVATE void ElevatorInUp(struct RequestQueue *queue, struct Request *request)
{
    struct Request *tmp = NULL;
    char flags = 0;
    
    /* 另外一个队列就是下请求 */
    struct List *anotherList = &queue->downRequestList;
    /* 插入到当前队列后面 */
    if (request->lba >= queue->currentRequest->lba) {
        ListForEachOwner(tmp, queue->requestList, queueList) {
            /* 找到一个旧请求的lba <= 请求的lba的请求 */
            if (request->lba >= tmp->lba) {
                /* find */
                flags = 1;
                break;
            }
        }
        /* 找到一个合适的位置，插入到该请求后面 */
        if (flags) {
            ListAddAfter(&request->queueList, &tmp->queueList);
        } else {
            /* 没有找到，就报错 */
            Panic("Find in up request failed!\n");
        }
    } else {
        /* 插入到另外队列中 */

        /* 如果是空队列，就直接插入在最前面 */
        if (ListEmpty(anotherList)) {
            ListAdd(&request->queueList, anotherList);

        } else {
            /* 搜索一个合适的位置并且插入 */
            ListForEachOwner(tmp, anotherList, queueList) {
                /* 找到一个旧请求的lba <= 请求的lba的请求 */
                if (request->lba <= tmp->lba) {
                    /* find */
                    flags = 1;
                    break;
                }
            }
            /* 找到一个合适的位置，插入到该请求后面 */
            if (flags) {
                ListAddAfter(&request->queueList, &tmp->queueList);
            } else {
                /* 没有找到比自己lba小的，就插入到另外的最前面 */
                ListAdd(&request->queueList, anotherList);

            }
        }
    }
}

/**
 * ElevatorInDown - 在下移电梯中进行调度处理
 * @queue: 请求队列
 * @request: 请求
 * 
 * 当当前的请求队列指向下移请求队列时，就调用该函数进行I/O调度处理
 */
PRIVATE void ElevatorInDown(struct RequestQueue *queue, struct Request *request)
{
    
    struct Request *tmp = NULL;
    char flags = 0;

    /* 另外一个队列就是上请求 */
    struct List *anotherList = &queue->upRequestList;
    
    /* 插入到当前队列后面 */
    if (request->lba <= queue->currentRequest->lba) {
        ListForEachOwner(tmp, queue->requestList, queueList) {
            /* 找到一个旧请求的lba <= 请求的lba的请求 */
            if (request->lba <= tmp->lba) {
                /* find */
                flags = 1;
                break;
            }
        }
        /* 找到一个合适的位置，插入到该请求后面 */
        if (flags) {
            ListAddAfter(&request->queueList, &tmp->queueList);
        } else {
            /* 没有找到，就插报错 */
            Panic("Find in down request failed!\n");
        }
    } else {
        /* 插入到另外队列中 */

        /* 如果是空队列，就直接插入在最前面 */
        if (ListEmpty(anotherList)) {
            ListAdd(&request->queueList, anotherList);
        } else {
            /* 搜索一个合适的位置并且插入 */
            ListForEachOwner(tmp, anotherList, queueList) {
                /* 找到一个旧请求的lba <= 请求的lba的请求 */
                if (request->lba >= tmp->lba) {
                    /* find */
                    flags = 1;
                    break;
                }
            }
            /* 找到一个合适的位置，插入到该请求后面 */
            if (flags) {
                ListAddAfter(&request->queueList, &tmp->queueList);
            } else {
                /* 没有找到比自己lba大的，就插入到另外的最前面 */
                ListAdd(&request->queueList, anotherList);
            }
        }
    }
}

/**
 * ElevatorIoSchedule - 电梯I/O调度
 * @queue: 队列
 * @request: 请求
 */
PUBLIC void ElevatorIoSchedule(struct RequestQueue *queue, struct Request *request)
{
    /* 调试信息
    if (queue->requestList == &queue->upRequestList) {
        //printk("up elevator\n");
    } else {
        //printk("down elevator\n");
    }
    if (queue->currentRequest == NULL) {
        //printk("->current request is null.\n");
    }*/

    /* 如果队列是空的，那么就直接插入到链表头 */
    if (ListEmpty(queue->requestList)) {
        ListAdd(&request->queueList, queue->requestList);
    } else {
        /* 进行调度 */
        /* 当前请求队列是上请求 */
        if (queue->requestList == &queue->upRequestList) {
            ElevatorInUp(queue, request);
        } else {    
            /* 如果是在下请求队列中 */
            ElevatorInDown(queue, request);
        }
    }
}
