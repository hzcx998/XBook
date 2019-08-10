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

/* 时钟频率 */
#define TIMER_QUICKEN     1
#define CLOCK_HAL_HZ             (100*TIMER_QUICKEN)	//1000 快速 100 普通0.001

//I/O 操作
#define CLOCK_HAL_IO_REGISTER_INT   1
#define CLOCK_HAL_IO_ENABLE_INT     2
#define CLOCK_HAL_IO_DISABLE_INT    3


#endif  //_HAL_CHAR_CLOCK_H
