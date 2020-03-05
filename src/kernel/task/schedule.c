/*
 * file:		kernel/task/schedule.c
 * auther:	    Jason Hu
 * time:		2019/7/30
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/schedule.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/task.h>
#include <book/list.h>
#include <lib/string.h>

/* 导入主idle线程 */
EXTERN Task_t *taskIdle;

EXTERN struct List taskPriorityQueue[MAX_PRIORITY_NR];

/** 
 * SwitchTo - 任务切换的核心
 * @prev: 当前任务
 * @next: 要切换过去的任务
 * 
 * 切换任务时保存当前环境，再选择新任务的环境去执行
 */
EXTERN void SwitchTo(struct Task *prev, struct Task *next);
/**
 * ScheduleInsertQueue - 插入到就绪队列
 * @task: 任务
 * 
 * 如果是运行的时候，表明任务还没执行完，所以会变成就绪状态，回到队列末尾
 * 如果是其它状态，表明任务现在不再运行，就不插入就绪队列。
 */
PRIVATE int ScheduleInsertQueue(Task_t *task)
{    
    /* 1.把自己加入就绪队列 */
    if (task->status == TASK_RUNNING) {
        /* 时间片到了，加入就绪队列 */
        TaskPriorityQueueAddTail(task);
        // 更新信息
        task->ticks = task->timeslice;
        task->status = TASK_READY;
        return 0;
    } else {
        /* 如果是需要某些事件后才能继续运行，不用加入队列，当前线程不在就绪队列中。*/        
        
        return -1;
    }
}

/**
 * ScheduleTryWakeupIdle - 尝试唤醒idle任务
 * 
 * 只有当没有进程可以运行的时候才唤醒它
 */
PRIVATE void ScheduleTryWakeupIdle()
{
    // 队列为空，那么就尝试唤醒idle
    if (IsAllPriorityQueueEmpty()) {
        // 唤醒mian(idle)
        TaskUnblock(taskIdle);
    }
}

/**
 * SchedulePickTask - 挑选一个任务来执行
 * 
 */
PRIVATE Task_t *SchedulePickTask()
{
    /* 一定能够找到一个任务，因为最后的是idle任务 */
    Task_t *task;
    int i;
    for (i = 0; i < MAX_PRIORITY_NR; i++) {
        /* 如果有任务，才获取 */
        if (!ListEmpty(&taskPriorityQueue[i])) {
            task = ListFirstOwner(&taskPriorityQueue[i], Task_t, list);
            ListDel(&task->list);
            break;
        }
    }
    return task;
}

/**
 * Schedule - 任务调度
 * 
 * 当需要从一个任务调度到另一个任务时，使用这个函数来执行操作
 * 
 * 把自己放入队列末尾。
 * 从优先级最高的队列中选取一个任务来执行。
 * 
 */
PUBLIC void Schedule()
{
    Task_t *current = CurrentTask();
    /* 1.插入到就绪队列 */
    ScheduleInsertQueue(current);

    /* 尝试唤醒idle任务 */
    ScheduleTryWakeupIdle();
    
    /* 2.从就绪队列中获取一个任务 */
    Task_t *next = SchedulePickTask();

    /* 3.激活任务的内存环境 */
    TaskActivate(next);

    /* 4.切换到该任务运行 */
    SwitchTo(current, next);
}
