/*
 * file:		kernel/schedule.c
 * auther:	    Jason Hu
 * time:		2019/7/30
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/schedule.h>
#include <book/arch.h>
#include <book/debug.h>
#include <share/string.h>
#include <book/task.h>

/* 导入主线程，也是idle线程 */
EXTERN Task_t *mainThread;

/** 
 * SwitchTo - 任务切换的核心
 * @prev: 当前任务
 * @next: 要切换过去的任务
 * 
 * 切换任务时保存当前环境，再选择新任务的环境去执行
 */
EXTERN void SwitchTo(struct Task *prev, struct Task *next);

/**
 * Schedule - 任务调度
 * 
 * 当需要从一个任务调度到另一个任务时，使用这个函数来执行操作
 */
PUBLIC void Schedule()
{
    //PART_START("Schedule");
    Task_t *current = CurrentTask();

    /* 1.把自己加入就绪队列 */
    if (current->status == TASK_RUNNING) {      
        /* 时间片到了，加入就绪队列 */
        
        //printk("-%s", current->name);
        // 检查链表情况
        // 保证不存在链表中
        ASSERT(!ListFind(&current->list, &taskReadyList));
        // 加入链表最后
        ListAddTail(&current->list, &taskReadyList);
        //
        // 更新信息
        current->ticks = current->priority;
        current->status = TASK_READY;
        
    } else {
        /* 如果是需要某些事件后才能继续运行，不用加入队列，当前线程不在就绪队列中。*/
        
    }

    // 队列为空，那么就尝试唤醒idle
    if (ListEmpty(&taskReadyList)) {
        // 唤醒mian(idle)
        TaskUnblock(mainThread);
    }
    
    /* 2.从就绪队列中获取一个任务 */
    ASSERT(!ListEmpty(&taskReadyList));

    // 获取链表中第一个宿主
    Task_t* next = ListFirstOwner(&taskReadyList, Task_t, list);
    
    // 把next从就绪队列中删除
    ListDel(&next->list);

    // 设置成运行状态
    next->status = TASK_RUNNING;

    //printk(PART_TIP "next  a name %s %d %d\n", next->name, ListLength(&TaskReadyList), (int)next->status);
    
    //printk("-next %s-", next->name);
    
    //printk("Switch from %s to %s\n", current->name, next->name);
    /*
    if (!strcmp(next->name,"init")) {
        //printk("!!!!\n!!!!");
    }*/

    /* 3.激活任务的内存环境 */
    TaskActivate(next);
    /* 4.切换到该任务运行 */
    SwitchTo(current, next);
}


