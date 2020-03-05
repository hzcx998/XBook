/*
 * file:		clock/pit/clock.c
 * auther:		Jason Hu
 * time:		2019/6/26
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/debug.h>
#include <book/interrupt.h>
#include <lib/math.h>
#include <lib/stddef.h>
#include <clock/clock.h>
#include <clock/pit/clock.h>

EXTERN void TimerSoftirqHandler(struct SoftirqAction *action);
EXTERN void SchedSoftirqHandler(struct SoftirqAction *action);

/* 时钟产生频率增快，默认是1，最大时10，表示增大10倍，现在的linux内核都是10
我们约定，当有图形界面的时候把它设置为5~10，没有的时候设置为1~5
 */

#define COUNTER0_VALUE  (TIMER_FREQ / HZ)	    //pit count0 数值

/**
 * ClockHandler - 时钟中断处理函数
 */
PRIVATE void ClockHandler(unsigned int irq, unsigned int data)
{
    /* 改变ticks计数 */
	systicks++;
	
	/* 激活定时器软中断 */
	ActiveSoftirq(TIMER_SOFTIRQ);

	/* 激活调度器软中断 */
	ActiveSoftirq(SCHED_SOFTIRQ);
	
}

/**
 * InitPitClockDriver - 初始化时钟管理
 */
PUBLIC void InitPitClockDriver()
{
	//初始化时钟
	Out8(PIT_CTRL, PIT_MODE_2 | PIT_MODE_MSB_LSB | 
            PIT_MODE_COUNTER_0 | PIT_MODE_BINARY);
	Out8(PIT_COUNTER0, (uint8_t) COUNTER0_VALUE);
	Out8(PIT_COUNTER0, (uint8_t) (COUNTER0_VALUE >> 8));
    
    /*Out8(PIT_COUNTER0, 0x9c);
	Out8(PIT_COUNTER0, 0x2e);
    printk("timer value:%x , %x\n", COUNTER0_VALUE, (0x2e << 8) | 0x9c);
    */
	systicks = 0;

	/* 注册定时器软中断处理 */
	BuildSoftirq(TIMER_SOFTIRQ, TimerSoftirqHandler);

	/* 注册定时器软中断处理 */
	BuildSoftirq(SCHED_SOFTIRQ, SchedSoftirqHandler);

	/* 注册时钟中断并打开中断 */	
	RegisterIRQ(IRQ0_CLOCK, &ClockHandler, IRQF_DISABLED, "clockirq", "clock", 0);

}
