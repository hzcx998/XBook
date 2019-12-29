/*
 * file:		kernel/alarm.c
 * auther:	    Jason Hu
 * time:		2019/12/27
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/debug.h>
#include <book/task.h>
#include <book/signal.h>
#include <drivers/clock.h>

/**
 * UpdateAlarmSystem - 更新闹钟系统
 * 
 * 由于闹钟的基本单位是秒，所以更新也是1s执行一次
 * 
 */
PUBLIC void UpdateAlarmSystem()
{
    struct Task *task;
    ListForEachOwner(task, &taskGlobalList, globalList) {
        if (task->alarm) {
            
            task->alarmTicks--;
            if (!task->alarmTicks) {
                //printk("a");
                task->alarmSeconds--;
                task->alarmTicks = HZ;
                /* 如果时间结束，那么就发送SIGALRM信号 */
                if (!task->alarmSeconds) {
                    //printk("alarm! %d\n", task->pid);
                    ForceSignal(SIGALRM, task->pid);
                    task->alarm = 0;
                }
            }
        }
    }
}

/**
 * SysAlarm - 设置闹钟
 * @seconds: 秒数
 * 
 * 设定闹钟，如果seconds是0，就取消闹钟
 * 
 * 返回上次剩余的秒数，也就是说，如果上次设置闹钟后在发生闹钟之前又设置了闹钟，
 * 此时，就会把上次剩余的描述返回。
 */
PUBLIC unsigned int SysAlarm(unsigned int seconds)
{
    struct Task *cur = CurrentTask();

    /* 需要返回上次剩余的秒数 */
    unsigned int oldSeconds = cur->alarmSeconds;

    if (seconds == 0) {
        cur->alarm = 0;     /* 取消闹钟 */
    } else {
        cur->alarm = 1;     /* 定时器生效 */
        cur->alarmSeconds = seconds;    /* 设定秒数 */
        cur->alarmTicks = HZ; /* 设置ticks数 */
        
        //printk("alarm seconds %d ticks %d\n", seconds, cur->alarmTicks);
    }
    //printk("alarm ret %d\n", oldSeconds);
    return oldSeconds;
}
