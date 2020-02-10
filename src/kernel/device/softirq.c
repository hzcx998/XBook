/*
 * file:		kernel/device/softirq.c
 * auther:	    Jason Hu
 * time:		2019/8/19
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/interrupt.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/bitops.h>
#include <lib/string.h>

/* 普通任务协助队列 */
struct TaskAssistHead taskAssistHead;

/* 高级任务协助队列 */
struct TaskAssistHead highTaskAssistHead;

/* 软中断表 */
PRIVATE struct SoftirqAction softirqTable[NR_SOFTIRQS];

/* 软中断事件的标志 */
PRIVATE unsigned int softirqEvens;

/**
 * GetSoftirqEvens - 获取软中断事件
 */
PRIVATE unsigned int GetSoftirqEvens()
{
    return softirqEvens;
}


/**
 * SetSoftirqEvens - 设置软中断事件
 */
PRIVATE void SetSoftirqEvens(unsigned int evens)
{
    softirqEvens = evens;
}

/**
 * BuildSoftirq - 构建软中断
 * @softirq: 软中断号
 * @action: 软中断的行为
 * 
 * 构建后，软中断才可以激活，才可以找到软中断行为处理
 */
PUBLIC void BuildSoftirq(unsigned int softirq, void (*action)(struct SoftirqAction *))
{
    /* 在范围内才进行设定 */
    if (0 <= softirq && softirq < NR_SOFTIRQS) {
        /* 把活动添加到软中断表中 */
        softirqTable[softirq].action = action;    
    }
}

/**
 * ActiveSoftirq - 激活软中断
 * @softirq: 软中断号
 * 
 * 激活后，每当有硬件中断产生，或者主动调用软中断处理的地方时，才会去执行软中断
 */
PUBLIC void ActiveSoftirq(unsigned int softirq)
{
    /* 在范围内才进行设定 */
    if (0 <= softirq && softirq < NR_SOFTIRQS) {
        /* 在对应位置修改软中断事件 */
        if (softirqTable[softirq].action)
            softirqEvens |= (1 << softirq);  
    }
}


/**
 * HandleSoftirq - 处理软中断
 */
PRIVATE void HandleSoftirq()
{
    struct SoftirqAction *action;

    unsigned int evens;
    /* 再次处理计数 */
    int redoIrq = MAX_REDO_IRQ;

    /* 获取软中断事件 */
    evens = GetSoftirqEvens();

/*如果在处理软中断过程中又产生了中断，就会导致事件变化，如果
量比较大，就在这里多做几次处理*/
ToRedo:
    
    /* 如果有事件，就会逐个运行 */
    if (evens) {
        /* 已经获取了，就置0 */
        SetSoftirqEvens(0);
        
        /* 如果有事件，就会逐个运行，由于事件运行可能会花费很多时间，
        所以开启中断，允许中断产生 */
        action = &softirqTable[0];
        
        /* 打开中断，允许中断产生 */
        EnableInterrupt();

        /* 处理softirq事件 */
        do {
            /* 如果有软中断事件 */
            if (evens & 1)
                action->action(action);     /* 执行软中断事件 */
            
            /* 指向下一个行为描述 */
            action++;
            /* 指向下一个事件 */
            evens >>= 1;
        } while(evens);

        /* 关闭中断 */
        DisableInterrupt();

        /* 检查是否还有事件没有处理，也就是说在处理事件过程中，又发生了中断 */
        evens = GetSoftirqEvens();
        
        /* 如果有事件，并且还可以继续尝试处理事件就继续运行 */
        if (evens && --redoIrq)
            goto ToRedo;
    }   
    /* 已经处理完了，可以返回了 */
}

/**
 * DoSoftirq - 做软中断处理
 * 
 * 通过这个地方，将有机会去处理软中断事件
 */
PUBLIC void DoSoftirq()
{
    unsigned int evens;

    /* 如果当前硬件中断嵌套，或者有软中断执行，则立即返回。*/

    /* 关闭中断 */
    enum InterruptStatus oldStatus = InterruptDisable();

    /* 获取事件 */
    evens = GetSoftirqEvens();

    /* 有事件就做软中断的处理 */
    if (evens) 
        HandleSoftirq();

    /* 恢复之前状态 */
    InterruptSetStatus(oldStatus);
}

/**
 * HighTaskAssistSchedule - 高级任务协助调度
 * @assist: 任务协助
 * 
 * 对一个任务协助进行调度，它后面才可以运行
 */
PUBLIC void HighTaskAssistSchedule(struct TaskAssist *assist)
{
    /* 如果状态还没有调度，才能进行调度 */
    if (!TestAndSetBit(TASKASSIST_SCHED, &assist->status)) {
        enum InterruptStatus oldStatus = InterruptDisable();

        /* 把任务协助插入到队列最前面 */
        assist->next = highTaskAssistHead.head;
        highTaskAssistHead.head = assist;

        /* 激活HIGHTTASKASSIST_SOFTIRQ */
        ActiveSoftirq(HIGHTASKASSIST_SOFTIRQ);

        InterruptSetStatus(oldStatus);
    }
}

/**
 * HighTaskAssistAction - 高级任务协助行为处理 
 * @action: 行为
 */
PRIVATE void HighTaskAssistAction(struct SoftirqAction *action)
{
    struct TaskAssist *list;

    /* 先关闭中断，不然修改链表可能和添加链表产生排斥 */
    DisableInterrupt();
    /* 获取链表头指针，用于寻找每一个调度的任务协助 */
    list = highTaskAssistHead.head;
    /* 把头置空，用于后面添加协助 */
    highTaskAssistHead.head = NULL;
    EnableInterrupt();

    /* 开始获取并处理协助 */
    while (list != NULL) {
        struct TaskAssist *assist = list;
        list = list->next;
        
        /* 协助处于打开状态(count == 0, enable) */
        if (!AtomicGet(&assist->count)) {
            /* 设置状态为空，不是调度状态 */
            ClearBit(TASKASSIST_SCHED, &assist->status);

            /* 执行协助的处理函数 */
            assist->func(assist->data);

            /* 如果协助可以运行，那么运行后就不运行后面重新添加到队列中，
            待下次运行 */
            continue;
        }

        /* 如果写成是关闭状态，那么就重新加入到队列 */
        
        /* 修改链表数据时禁止中断 */
        DisableInterrupt();
        /* 把任务协助插入到队列最前面 */
        assist->next = highTaskAssistHead.head;
        highTaskAssistHead.head = assist;

        /* 激活HIGHTASKASSIST_SOFTIRQ */
        ActiveSoftirq(HIGHTASKASSIST_SOFTIRQ);
        
        EnableInterrupt();
        
    }
}

/**
 * TaskAssistSchedule - 普通任务协助调度
 * @assist: 任务协助
 * 
 * 对一个任务协助进行调度，它后面才可以运行
 */
PUBLIC void TaskAssistSchedule(struct TaskAssist *assist)
{
    /* 如果状态还没有调度，才能进行调度 */
    if (!TestAndSetBit(TASKASSIST_SCHED, &assist->status)) {
        enum InterruptStatus oldStatus = InterruptDisable();

        /* 把任务协助插入到队列最前面 */
        assist->next = taskAssistHead.head;
        taskAssistHead.head = assist;

        /* 激活TASKASSIST_SOFTIRQ */
        ActiveSoftirq(TASKASSIST_SOFTIRQ);

        InterruptSetStatus(oldStatus);
    }
}


/**
 * TaskAssistAction - 普通任务协助行为处理 
 * @action: 行为
 */
PRIVATE void TaskAssistAction(struct SoftirqAction *action)
{
    struct TaskAssist *list;

    /* 先关闭中断，不然修改链表可能和添加链表产生排斥 */
    DisableInterrupt();
    /* 获取链表头指针，用于寻找每一个调度的任务协助 */
    list = taskAssistHead.head;
    /* 把头置空，用于后面添加协助 */
    taskAssistHead.head = NULL;
    EnableInterrupt();

    /* 开始获取并处理协助 */
    while (list != NULL) {
        struct TaskAssist *assist = list;
        list = list->next;
        
        /* 协助处于打开状态(count == 0, enable) */
        if (!AtomicGet(&assist->count)) {
            /* 设置状态为空，不是调度状态 */
            ClearBit(TASKASSIST_SCHED, &assist->status);

            /* 执行协助的处理函数 */
            assist->func(assist->data);

            /* 如果协助可以运行，那么运行后就不运行后面重新添加到队列中，
            待下次运行 */
            continue;
        }

        /* 如果写成是关闭状态，那么就重新加入到队列 */
        
        /* 修改链表数据时禁止中断 */
        DisableInterrupt();
        /* 把任务协助插入到队列最前面 */
        assist->next = taskAssistHead.head;
        taskAssistHead.head = assist;

        /* 激活TASKASSIST_SOFTIRQ */
        ActiveSoftirq(TASKASSIST_SOFTIRQ);
        
        EnableInterrupt();
        
    }
}

/**
 * InitSoftirq - 初始化软中断
 */
PUBLIC void InitSoftirq()
{
    /* 初始化软中断事件 */
    softirqEvens = 0;

    /* 设置高级任务协助头为空 */
    highTaskAssistHead.head = NULL;
    
    /* 设置普通任务协助头为空 */
    taskAssistHead.head = NULL;
    
    /* 注册高级任务协助软中断 */
    BuildSoftirq(HIGHTASKASSIST_SOFTIRQ, HighTaskAssistAction);

    /* 注册普通任务协助软中断 */
    BuildSoftirq(TASKASSIST_SOFTIRQ, TaskAssistAction);
    
}
