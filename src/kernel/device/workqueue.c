/*
 * file:		kernel/device/workqueue.c
 * auther:	    Jason Hu
 * time:		2019/8/17
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/interrupt.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/mmu.h>
#include <book/task.h>
#include <book/schedule.h>
#include <lib/string.h>

/* 工作队列链表，用来维护所有的工作队列 */
struct List workQueueList;


struct WorkQueue *keventWorkQueue;

/**
 * DoWorkQueue - 处理工作
 * @cpuWorkQueu: 工作所在的cpu
 */
PRIVATE void DoAllWork(struct CpuWorkQueue *cpuWorkQueue)
{

    /* 如果工作队列中有工作，就进行处理 */
    while (!ListEmpty(&cpuWorkQueue->workList)) {
        struct Work *work;
        
        WrokFunc_t func;
        /* 获取第一个工作，通过工作的list找到work */
        work = ListOwner(cpuWorkQueue->workList.next, struct Work, list);

        func = work->func;
        /* 从工作队列中删除当前工作 */
        ListDelInit(&work->list);

        /* 执行工作 */
        func(work);
    }
}

/**
 * WorkerThread - 工作者线程
 * @__cpuWorkQueue: cpu工作队列
 */
PRIVATE void WorkerThread(void *__cpuWorkQueue)
{
    struct CpuWorkQueue *cpuWorkQueue = __cpuWorkQueue;
    struct Task *current = CurrentTask();
    // printk("worker\n");
    while (1) {
        /* 如果队列为空，就休眠 */
        WaitQueueAdd(&cpuWorkQueue->moreWork, current);
        /* 先把状态设置成阻塞，这样调度后就不会再次被调度，只有等待唤醒 */
        current->status = TASK_BLOCKED;

        if (ListEmpty(&cpuWorkQueue->workList)) {
            //printk("empty thread %s blocked\n", cpuWorkQueue->thread->name);
            
            // 任务调度，不运行
            Schedule();
        }
        
        /* 之前如果工作为空就会休眠，直到被唤醒。到这里的时候，要么是唤醒后，
        要么是没休眠直接到这，把任务从等待队列删除 */
        WaitQueueRemove(&cpuWorkQueue->moreWork, current);
        /* 再把状态设置成运行 */
        current->status = TASK_RUNNING;
        
        /* 执行工作 */
        DoAllWork(cpuWorkQueue);
    }
}

/**
 * CreateWorkQueue - 创建一个工作队列
 * @name: 工作队列的名字
 * 
 * 创建后，会运行一个daemon内核线程，用来处理工作
 */
PUBLIC struct WorkQueue *CreateWorkQueue(const char *name)
{
    /* 为工作队列分配一块内存 */
    struct WorkQueue *workQueue = kmalloc(WORK_QUEUE_SIZE, GFP_KERNEL);
    if (workQueue == NULL) 
        return NULL;
    
    /* 设置名字 */
    workQueue->name = name;
    
    /* 初始化链接所有工作队列的链表 */
    INIT_LIST_HEAD(&workQueue->list);

    /* 获取cpu的工作队列 */
    struct CpuWorkQueue *cpuWorkQueue = &workQueue->cpuWorkQueue;

    /* 初始化所有工作的链表头 */
    INIT_LIST_HEAD(&cpuWorkQueue->workList);

    /* 当前没有处理中的工作 */
    cpuWorkQueue->currentWork = NULL;

    /* 获取自己的工作队列 */
    cpuWorkQueue->myWorkQueue = workQueue;
    //
    // printk("ALLOC");
    /* 需要创建一个线程来处理 */
    cpuWorkQueue->thread = ThreadStart((char *)name, TASK_WORKER_PRIO, WorkerThread, cpuWorkQueue);   
    
    /* 初始化等待队列 */
    WaitQueueInit(&cpuWorkQueue->moreWork, NULL);

    /* 把工作队列添加到工作队列链表 */
    ListAddTail(&workQueue->list, &workQueueList);
    //printk("CreateWorkQueue: wq(%x) %s thread(%x) %s\n", workQueue, workQueue->name, cpuWorkQueue->thread, cpuWorkQueue->thread->name);
    
    return workQueue;
}

/**
 * DestroyWorkQueue - 销毁一个工作队列
 * @workQueue: 工作队列
 * 
 * 在这里，我们不销毁缺省的工作队列，只销毁自己创建的工作队列
 */
PUBLIC void DestroyWorkQueue(struct WorkQueue *workQueue)
{
    if (!workQueue)
        return;

    struct CpuWorkQueue *cpuWorkQueue = &workQueue->cpuWorkQueue;

    /* 保证工作队列没有工作，并且处于阻塞状态才能进行销毁 */
    FlushWorkQueue(workQueue);

    /* 删除工作队列期间要关闭中断，防止产生新的工作 */
    enum InterruptStatus oldStatus = InterruptDisable();

    /* 从工作队列链表删除 */
    ListDel(&workQueue->list);

    /* 如果线程在等待队列里面，就把等待队列中的线程删除 */
    WaitQueueRemove(&cpuWorkQueue->moreWork, cpuWorkQueue->thread);

    /* 线程退出运行 */
    ThreadExit(cpuWorkQueue->thread);

    InterruptSetStatus(oldStatus);
    /* 释放工作队列结构体 */
    kfree(workQueue);
}

/**
 * QueueScheduleWork - 队列调度工作
 * @workQueue: 工作队列
 * @work: 工作
 * 
 * 把工作添加到工作队列中去
 */
PUBLIC int QueueScheduleWork(struct WorkQueue *workQueue, struct Work *work)
{
    /* 先检测传递的参数是否有问题 */
    if (!workQueue || !work)
        return -1;

    struct CpuWorkQueue *cpuWorkQueue = &workQueue->cpuWorkQueue;
    /* 1.检测工作是否已经在工作队列之中 */
    if (ListFind(&work->list, &cpuWorkQueue->workList)) 
        return -1; 
    
    /* 2.把工作添加到工作队列中 */

    /* 修改链表的时候应该关闭中断 */
    //enum InterruptStatus oldStatus = InterruptDisable();

    uint32_t eflags = LoadEflags();
    DisableInterrupt();

    /* 先后队列排序，添加到队列尾部 */
    ListAddTail(&work->list, &cpuWorkQueue->workList);

    StoreEflags(eflags);
    
    //InterruptSetStatus(oldStatus);
    
    /* 3.尝试唤醒工作者线程 */
    
    //printk("QueueScheduleWork: wq(%x) %s thread(%x) %s\n", workQueue, workQueue->name, cpuWorkQueue->thread, cpuWorkQueue->thread->name);

    /* 唤醒等待队列中的一个任务，如果队列为空就啥也不做 */
    WaitQueueWakeUp(&cpuWorkQueue->moreWork);
    
    return 0;
}

/**
 * ScheduleWork - 调度工作到缺省的工作队列
 * @wrok: 要调度的工作
 * 
 * 调度完成后，events线程会工作，来处理这些工作
 */
PUBLIC int ScheduleWork(struct Work *work)
{
    //printk("schedule work\n");
    return QueueScheduleWork(keventWorkQueue, work);
}

/**
 * FlushWorkQueue - 刷新工作队列
 */
PUBLIC void FlushWorkQueue(struct WorkQueue *workQueue)
{
    if (!workQueue)
        return;

    struct CpuWorkQueue *cpuWorkQueue = &workQueue->cpuWorkQueue;
    
    /* 如果还有工作就休眠，直到没有工作才返回 */
    while (!ListEmpty(&cpuWorkQueue->workList)) {
        /* 应该休眠，在调度完所有工作后，查看是否有休眠者，有的话就唤醒休眠者
        这里简单地使用了yield
         */
        TaskYield();
    }
    /* 工作队列的工作都做完了，现在返回 */
}

/**
 * FlushScheduleWork - 刷新缺省的工作队列
 * 
 */
PUBLIC void FlushScheduleWork()
{
    FlushWorkQueue(keventWorkQueue);
}

/**
 * PrintWorkQueue - 打印工作队列
 */
PUBLIC void PrintWorkQueue()
{

    printk(PART_TIP "\n----Work Queue----\n");
    struct WorkQueue *workQueue;
    ListForEachOwner(workQueue, &workQueueList, list) {
        printk("name %s thread %s\n", workQueue->name, workQueue->cpuWorkQueue.thread->name);
    }

}

/**
 * InitWorkQueue - 初始化工作队列
 */
PUBLIC void InitWorkQueue()
{
    PART_START("Work queue");

    /* 初始化工作队列链表头 */
    INIT_LIST_HEAD(&workQueueList);

    /* 创建缺省工作者线程 */
    keventWorkQueue = CreateWorkQueue("events");
    //printk("work queue at %x\n", keventWorkQueue);
    PART_END();
}
