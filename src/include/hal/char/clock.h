/*
 * file:		include/hal/char/clock.h
 * auther:		Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _HAL_CHAR_CLOCK_H
#define _HAL_CHAR_CLOCK_H

#include <share/stdint.h>
#include <share/types.h>
#include <book/hal.h>


#define PIT_CTRL	0x0043
//控制端口
#define PIT_CNT0	0x0040
//数据端口
#define TIMER_FREQ     1193180	
#define TIMER_QUICKEN     5
#define HZ             (100*TIMER_QUICKEN)	//1000 快速 100 普通0.001

void ClockHalInit();

#endif  //_HAL_CHAR_CLOCK_H
