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

int ticks;

PRIVATE void ClockHnadler()
{
	ticks++;
	if ((ticks % HZ) == 0) {
		printk("%x ", ticks);
	}
}

PUBLIC void ClockInit()
{
	PART_START("Clock driver");
	
	//打开时钟硬件抽象
	HalOpen("clock");

	// 注册时钟中断并打开中断
	HalIoctl("clock", CLOCK_HAL_IO_REGISTER_INT, (unsigned int)&ClockHnadler);
	HalIoctl("clock", CLOCK_HAL_IO_ENABLE_INT, 0);

	ticks = 0;

	// 打开中断标志
	InterruptEnable();

	PART_END();
}
