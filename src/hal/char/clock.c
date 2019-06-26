/*
 * file:		hal/char/clock.c
 * auther:		Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <hal/hal.h>
#include <book/arch.h>
#include <book/debug.h>

/*
 * 对时钟的抽象，做了什么？
 * ->在初始化中，对时钟的环境进行初始化
 * ->提供了一些硬件相关的操作
 */

//控制端口
#define PIT_CTRL	0x0043

//数据端口
#define PIT_CNT0	0x0040

//时钟的频率
#define TIMER_FREQ     1193180	

PRIVATE void ClockHalIoctl(unsigned int type, unsigned int value)
{
	//根据类型设置不同的值
	switch (type)
	{
	case CLOCK_HAL_IO_REGISTER_INT:
		
		IrqRegisterHandler(IRQ0_CLOCK, (intr_handler_t )value);
		break;
	case CLOCK_HAL_IO_ENABLE_INT:
		
		EnableIRQ(IRQ0_CLOCK);
		break;
	case CLOCK_HAL_IO_DISABLE_INT:

		DisableIRQ(IRQ0_CLOCK);
		break;
	default:
		break;
	}
}

PRIVATE void ClockHalInit()
{
	//printk("-> clock hal ");
	PART_START("Clock hal");

	//0.001秒触发一次中断
	Out8(PIT_CTRL, 0x34);
	Out8(PIT_CNT0, (unsigned char) (TIMER_FREQ/CLOCK_HAL_HZ));
	Out8(PIT_CNT0, (unsigned char) ((TIMER_FREQ/CLOCK_HAL_HZ) >> 8));

	PART_END();
}

/* hal操作函数 */
PRIVATE struct HalOperate halOperate = {
	.Init = &ClockHalInit,
	.Ioctl = &ClockHalIoctl,
};

/* hal对象，需要把它导入到hal系统中 */
PUBLIC HAL(halOfClock, halOperate, "clock");