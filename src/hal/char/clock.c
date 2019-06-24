/*
 * file:		hal/char/display.c
 * auther:		Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <hal/hal.h>
#include <book/arch.h>
#include <book/debug.h>

int ticks;
void ClockHnadler();

void ClockHalInit()
{
	printk("> init clock hal start.\n");
	
	//0.001秒触发一次中断
	Out8(PIT_CTRL, 0x34);
	Out8(PIT_CNT0, (unsigned char) (TIMER_FREQ/HZ));
	Out8(PIT_CNT0, (unsigned char) ((TIMER_FREQ/HZ) >> 8));

	ticks = 0;

	InterruptRegisterHandler(0x20, ClockHnadler);
	EnableIRQ(0);
	
	printk("< step clock hal done.\n");
	
	//print_time();
	InterruptEnable();
}

void ClockHnadler()
{
	ticks++;
	if ((ticks % HZ) == 0) {
		printk("t:%x ", ticks);
	}
}
