/*
 * file:		drivers/char/timer.c
 * auther:		Jason Hu
 * time:		2019/8/8
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <book/arch.h>
#include <book/debug.h>
#include <drivers/clock.h>
#include <drivers/timer.h>
#include <book/schedule.h>
#include <book/task.h>

/* 定时器链表 */
PUBLIC struct List timerList;

/**
 * TimerInit - 初始化一个定时器
 * @timer: 定时器
 * @expires: 时间（ticks为单位）
 * @data: 传递的参数
 * @function: 要执行的函数
 */
PUBLIC void TimerInit(struct Timer *timer, uint64_t expires, uint32_t data,
        void (*function)(uint32_t))
{
    /* 初始化链表 */
	INIT_LIST_HEAD(&timer->list);

	timer->expires = expires;
	timer->data = data;
	timer->function = function;
	
    timer->state = TIMER_IDLE;

    timer->next = NULL;
}

PUBLIC void DoTimerHandler(struct Timer *timer)
{
    /* 执行完后应该把定时器从链表中删除，以后不再执行 */
    RemoveTimer(timer);
    timer->function(timer->data);
}

/**
 * TimerUpdate - 定时器更新
 * @timer: 要更新的定时器
 * 
 * 如果定时器到期了，就执行定时器里面的函数，并传递参数过去
 * 不然就直接返回
 */
PUBLIC bool TimerUpdate(struct Timer *timer)
{
    /* 运行中才更新 */
    if (timer->state == TIMER_RUNNING) {
        
        /* 减少计数 */
        --timer->expires;

        
        /* 定时器到期就执行函数 */
        if (timer->expires == 0){
            DoTimerHandler(timer);

            return true;
        }
    }
	return false;
}

/**
 * AddTimer - 添加一个定时器
 * @timer: 定时器
 * 
 * 添加定时器的时候，可能执行到一半就发生时钟中断
 * 所以要先关闭中断，避免获取定时器出错
 */
PUBLIC void AddTimer(struct Timer *timer)
{
	/* 保存状态并关闭中断 */
    enum InterruptStatus oldStatus = InterruptDisable();
    
	/* 保证定时器不在队列里面 */
	ASSERT(!ListFind(&timer->list, &timerList));

    timer->state = TIMER_RUNNING;

	/* 添加到队列 */
	ListAddTail(&timer->list, &timerList);

	/* 恢复之前的中断状态 */
	InterruptSetStatus(oldStatus);
}

/**
 * RemoveTimer - 移除一个定时器
 * @timer: 定时器
 */
PUBLIC void RemoveTimer(struct Timer *timer)
{
	/* 保存状态并关闭中断 */
    enum InterruptStatus oldStatus = InterruptDisable();
    
	/* 保证定时器在队列里面 */
	ASSERT(ListFind(&timer->list, &timerList));
	/* 从队列中删除 */
	ListDelInit(&timer->list);

	/* 恢复之前的中断状态 */
	InterruptSetStatus(oldStatus);
}

/**
 * StopTimer - 停止一个定时器
 * @timer: 定时器
 */
PUBLIC void StopTimer(struct Timer *timer)
{
	/* 保存状态并关闭中断 */
    enum InterruptStatus oldStatus = InterruptDisable();
    
    /* 设置成停止状态 */
    timer->state = TIMER_STOP;

	/* 恢复之前的中断状态 */
	InterruptSetStatus(oldStatus);
}

/**
 * ResumeTimer - 继续运行定时器
 * @timer: 定时器
 */
PUBLIC void ResumeTimer(struct Timer *timer)
{
	/* 保存状态并关闭中断 */
    enum InterruptStatus oldStatus = InterruptDisable();
    
    /* 设置成停止状态 */
    timer->state = TIMER_RUNNING;
    
	/* 恢复之前的中断状态 */
	InterruptSetStatus(oldStatus);
}

/**
 * CancelTimer - 取消一个定时器
 * @timer: 定时器
 */
PUBLIC void CancelTimer(struct Timer *timer)
{
    RemoveTimer(timer); // 移除定时器

}



/**
 * UpdateTimerSystem - 更新定时器系统
 */
PUBLIC void UpdateTimerSystem()
{
    enum InterruptStatus oldStatus = InterruptDisable();
	struct Timer *timer, *next;
	/* 因为有可能会在更新后把定时器删除，所以这里要用安全 */
	ListForEachOwnerSafe(timer, next, &timerList, list) {
		TimerUpdate(timer);
	}
    InterruptSetStatus(oldStatus);
}

PRIVATE void TimerTest(uint32_t data)
{
	printk("\n\ntimer!!! %x\n\n", data);
}


/**
 * InitTimer - 初始化定时器
 */
PUBLIC void InitTimer()
{
	/* 初始化定时器链表头 */
	INIT_LIST_HEAD(&timerList);

	/*
	struct Timer *timer = kmalloc(sizeof(struct Timer), GFP_KERNEL);
	if (timer == NULL) {
		Panic(PART_TIP "kmalloc for timer failed!\n");
	}
	TimerInit(timer, 200, 10, TimerTest);
	AddTimer(timer);*/
}