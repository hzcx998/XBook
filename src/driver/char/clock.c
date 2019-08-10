/*
 * file:		driver/char/clock.c
 * auther:		Jason Hu
 * time:		2019/6/26
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <book/arch.h>
#include <book/debug.h>
#include <driver/clock.h>
#include <hal/char/clock.h>
#include <book/schedule.h>
#include <book/task.h>
#include <driver/timer.h>
#include <share/math.h>

PRIVATE int ticks;

/**
 * ClockHnadler - 时钟中断处理函数
 */
PRIVATE void ClockHnadler()
{
	struct Task *current = CurrentTask();

	/* 更新定时器 */
	UpdateTimerSystem();

	/* 更新任务调度 */
	current->elapsedTicks++;
	ticks++;
	
	if (current->ticks <= 0) {
		//printk("S ");
		
		Schedule();
	} else {
		//printk("T ");
		
		current->ticks--;
	}
	
}

/**
 * SleepByTicks - 休眠ticks时间
 * @sleepTicks: 要休眠多少个ticks
 */
PRIVATE void SleepByTicks(uint32_t sleepTicks)
{
	/* 获取起始ticks，后面进行差值运算 */
	uint32_t startTicks = ticks;

	/* 如果最新ticks和开始ticks差值小于要休眠的ticks，就继续休眠 */
	while (ticks - startTicks < sleepTicks) {
		/* 让出cpu占用，相当于休眠 */
		TaskYield();
	}
}

/**
 * SysMSleep - 以毫秒为单位进行休眠
 */
PUBLIC void SysMSleep(uint32_t msecond)
{
	/* 把毫秒转换成ticks */
	uint32_t sleepTicks = DIV_ROUND_UP(msecond, MILLISECOND_PER_TICKS);
	ASSERT(sleepTicks > 0);
	/* 进行休眠 */
	SleepByTicks(sleepTicks);
}

/**
 * InitClock - 初始化时钟管理
 */
PUBLIC void InitClock()
{
	PART_START("Clock driver");
	
	//打开时钟硬件抽象
	HalOpen("clock");

	// 注册时钟中断并打开中断
	HalIoctl("clock", CLOCK_HAL_IO_REGISTER_INT, (unsigned int)&ClockHnadler);
	HalIoctl("clock", CLOCK_HAL_IO_ENABLE_INT, 0);

	ticks = 0;

	/* 初始化定时器 */
	InitTimer();
	
	// 打开中断标志
	InterruptEnable();

	PART_END();
}
