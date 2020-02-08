/*
 * file:		include/book/interrupt.h
 * auther:		Jason Hu
 * time:		2019/8/17
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_INTERRUPT_H
#define _BOOK_INTERRUPT_H

#include <share/types.h>
#include <book/arch.h>
#include <book/atomic.h>
#include <book/list.h>
#include <book/task.h>
#include <book/waitqueue.h>
#include <book/timer.h>

/* ----中断上半部分---- */
/* IRQ 编号 */
enum {
    IRQ0 = 0,
    IRQ1,
    IRQ2,
    IRQ3,
    IRQ4,
    IRQ5,
    IRQ6,
    IRQ7,
    IRQ8,
    IRQ9,
    IRQ10,
    IRQ11,
    IRQ12,
    IRQ13,
    IRQ14,
    IRQ15,
    NR_IRQS
};


#define IRQF_DISABLED       0x01
#define IRQF_SHARED         0x02
#define IRQF_TIMER          0x03

/* irq对应的处理 */
struct IrqAction {
    unsigned int data;
    void (*handler)(unsigned int irq, unsigned int data);
    unsigned int flags;
    struct IrqAction *next;     // 指向下一个行动

    /* 表示设备名字 */
    char *name;
};

/* 硬件相关的操作 */
struct HardwareIntController {
    void (*enable)(unsigned int irq);
    void (*disable)(unsigned int irq);
    unsigned int (*install)(unsigned int irq, void *arg);
    void (*uninstall)(unsigned int irq);
    /* 接收到中断后确定中断已经接收 */ 
    void (*ack)(unsigned int irq);
};

struct IrqDescription {
    /* 硬件控制器，用来控制中断的硬件底层操作 */
    struct HardwareIntController *controller;
    struct IrqAction *action;
    unsigned int flags;
    Atomic_t deviceCount;       // 记录注册的设备数量

    /* 表示irq名字 */
    char *irqname;
};

PUBLIC INIT void InitIrqDescription();

PUBLIC int RegisterIRQ(unsigned int irq,
    void (*handler)(unsigned int irq, unsigned int data), 
    unsigned int flags,
    char *irqname,
    char *devname,
    unsigned int data);

PUBLIC int UnregisterIRQ(unsigned int irq, unsigned int data);


PUBLIC int HandleIRQ(unsigned int irq, struct TrapFrame *frame);


/* ----中断下半部分----*/

/* 软中断 */

/* 最大的软中断数量 */
#define MAX_NR_SOFTIRQS     32

#define MAX_REDO_IRQ     10

/* 定义软中断类型 */
enum
{
    HIGHTASKASSIST_SOFTIRQ = 0,
    TIMER_SOFTIRQ,
    NET_TX_SOFTIRQ,
    NET_RX_SOFTIRQ,
    TASKASSIST_SOFTIRQ,
    SCHED_SOFTIRQ, 
    RCU_SOFTIRQ,    /* Preferable RCU should always be the last softirq */
    NR_SOFTIRQS
};


/* 软中断行为 */
struct SoftirqAction {
    void (*action)(struct SoftirqAction *);
};


/* 软中断处理函数原型
void SoftirqHandler(struct SoftirqAction *action);
*/

PUBLIC void BuildSoftirq(unsigned int softirq, void (*action)(struct SoftirqAction *));
PUBLIC void ActiveSoftirq(unsigned int softirq);

PUBLIC void InitSoftirq();

/* TaskAssist, 任务协助 */

#define TASKASSIST_SCHED       0

struct TaskAssist {
    struct TaskAssist *next;        // 指向下一个任务协助
    unsigned int status;            // 状态
    Atomic_t count;                 // 任务协助的开启与关闭，0（enable），1（disable）
    void (*func)(unsigned int);     // 要执行的函数
    unsigned int data;              // 回调函数的参数
};

struct TaskAssistHead {
    struct TaskAssist *head;
};

/* 任务协助处理函数原型
void TaskAssistHandler(unsigned int);
*/

PUBLIC void TaskAssistSchedule(struct TaskAssist *assist);
PUBLIC void HighTaskAssistSchedule(struct TaskAssist *assist);
/**
 * TaskAssistInit - 初始化一个任务协助
 * @assist: 任务协助的地址
 * @func: 要处理的函数
 * @data: 传入的参数
 */
PRIVATE INLINE void TaskAssistInit(struct TaskAssist *assist, 
        void (*func)(unsigned int), unsigned int data)
{
    assist->next = NULL;
    assist->status = 0;
    AtomicSet(&assist->count, 0);
    assist->func = func;
    assist->data = data;
}

/* 初始化一个任务协助，使能的，enable */
#define DECLEAR_TASKASSIST(name, func, data) \
    struct TaskAssist name = {NULL, 0, ATOMIC_INIT(0),func, data}


/* 初始化一个任务协助，不使能的，disable */
#define DECLEAR_TASKASSIST_DISABLED(name, func, data) \
    struct TaskAssist name = {NULL, 0, ATOMIC_INIT(1),func, data}

/**
 * TaskAssistDisable - 不让任务协助生效
 * @assist: 任务协助
 */
PRIVATE INLINE void TaskAssistDisable(struct TaskAssist *assist)
{
    /* 减小次数，使其disable */
    AtomicDec(&assist->count);
}

/**
 * TaskAssistEnable - 让任务协助生效
 * @assist: 任务协助
 */
PRIVATE INLINE void TaskAssistEnable(struct TaskAssist *assist)
{
    /* 增加次数，使其enable */
    AtomicInc(&assist->count);
}

/* ----工作队列---- */

/* 工作的函数形式 */

typedef struct Work {
    /* 这里的数据不是函数的参数，如果要传入参数，
    需要用另外一个结构体包括本结构，然后用container_of获取参数 */
    unsigned int data;
    struct List list;   // list用来连接所有工作的队列
    void (*func)(struct Work *work);    // 要执行的函数
} Work_t;

typedef void (*WrokFunc_t)(struct Work *work);

struct CpuWorkQueue {
    struct List workList;   // 工作的链表头
    struct WaitQueue moreWork;  // 更多的工作
    struct Work *currentWork;   // 指向当前的工作
    struct WorkQueue *myWorkQueue;  // 所属的工作队列 
    struct Task *thread;    // 工作队列的内核线程      
};

typedef struct WorkQueue {
    struct CpuWorkQueue cpuWorkQueue;  // 对应cpu的工作队列
    struct List list;   // 所有的工作队列所在的链表
    const char *name;   // 工作队列的名字
} WorkQueue_t;

#define WORK_QUEUE_SIZE sizeof(struct WorkQueue)

/*
工作函数原型
void WorkHandler(struct Work *data);
*/
PUBLIC void InitWorkQueue();

PUBLIC struct WorkQueue *CreateWorkQueue(const char *name);
PUBLIC void DestroyWorkQueue(struct WorkQueue *workQueue);

PUBLIC int QueueScheduleWork(struct WorkQueue *workQueue, struct Work *work);
PUBLIC void FlushWorkQueue(struct WorkQueue *workQueue);

PUBLIC int ScheduleWork(struct Work *work);
PUBLIC void FlushScheduleWork();

PUBLIC void PrintWorkQueue();

/**
 * DECLEAR_WORK - 声明一个工作
 * @name: 工作的名字
 * @func: 工作要执行的函数
 */
#define DECLEAR_WORK(name, func) \
    struct Work name = {0, LIST_HEAD_INIT(name.list), func}

/**
 * WorkInit - 工作的初始化
 * @work: 工作
 * @func: 工作要执行的函数
 * 
 * 对工作进行初始化，设置运行函数
 */
PRIVATE INLINE void WorkInit(struct Work *work, WrokFunc_t func)
{
    work->data = 0;
    INIT_LIST_HEAD(&work->list);
    work->func = func;
}

#endif   /*_BOOK_INTERRUPT_H*/
