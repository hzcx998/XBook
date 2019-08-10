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

/**
 * 定时器结构
 */
struct Timer {
    uint64_t expires;   // 到期时间
    uint32_t data;      // 传递的数据
    void (*function)(uint32_t);     // 到期后要调用的函数
    struct List list;   // 链表
};


PUBLIC void InitTimer();
PUBLIC void TimerInit(struct Timer *timer, uint64_t expires, uint32_t data,
        void (*function)(uint32_t));

PUBLIC bool TimerUpdate(struct Timer *timer);

PUBLIC void UpdateTimerSystem();

PUBLIC void AddTimer(struct Timer *timer);
PUBLIC void RemoveTimer(struct Timer *timer);

#endif  /* _DRIVER_CHAR_TIMER_H */
