/*
 * file:		include/driver/timer.h
 * auther:		Jason Hu
 * time:		2019/8/8
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

/**
 * 定时器驱动，建立在时钟驱动之上
 */

#ifndef _DRIVER_CHAR_TIMER_H
#define _DRIVER_CHAR_TIMER_H

#include <share/stdint.h>
#include <share/types.h>
#include <book/list.h>
#include <book/atomic.h>

#define TIMER_IDLE         1       /* 定时器停机状态 */
#define TIMER_RUNNING      2       /* 定时器处于运行中 */
#define TIMER_STOP         3       /* 定时器暂停 */
#define TIMER_INVALID      4       /* 定时器无效 */

/**
 * 定时器结构
 */
struct Timer {
    struct List list;   // 链表
    uint64_t expires;   // 到期时间
    uint32_t data;      // 传递的数据
    void (*function)(uint32_t);     // 到期后要调用的函数
    char state;         // 定时器的状态
    struct Timer *next; // 定时器链表，用于在task中遍历task所属的定时器
};

PUBLIC void InitTimer();
PUBLIC void TimerInit(struct Timer *timer, uint64_t expires, uint32_t data,
        void (*function)(uint32_t));

PUBLIC bool TimerUpdate(struct Timer *timer);

PUBLIC void UpdateTimerSystem();

PUBLIC void AddTimer(struct Timer *timer);
PUBLIC void RemoveTimer(struct Timer *timer);
PUBLIC void StopTimer(struct Timer *timer);
PUBLIC void ResumeTimer(struct Timer *timer);
PUBLIC void CancelTimer(struct Timer *timer);

PUBLIC void DoTimerHandler(struct Timer *timer);

#endif  /* _DRIVER_CHAR_TIMER_H */
