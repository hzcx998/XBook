/*
 * file:		include/clock/pit/clock.h
 * auther:		Jason Hu
 * time:		2020/2/10
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_PIT_CLOCK_H
#define _DRIVER_PIT_CLOCK_H

#include <lib/stdint.h>
#include <lib/types.h>

#define TIMER_FREQ     1193180 	//时钟的频率

/*
PUBLIC void InitPitClockDriver();
*/
PUBLIC void SysMSleep(unsigned int msecond);

#endif  /* _DRIVER_ */
