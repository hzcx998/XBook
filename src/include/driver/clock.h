/*
 * file:		include/driver/clock.h
 * auther:		Jason Hu
 * time:		2019/6/26
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_CHAR_CLOCK_H
#define _DRIVER_CHAR_CLOCK_H

#include <share/stdint.h>
#include <share/types.h>
#include <hal/char/clock.h>

#define HZ  CLOCK_HAL_HZ

/* 1 ticks 对应的毫秒数 */
#define MILLISECOND_PER_TICKS (1000/ HZ)

PUBLIC void InitClock();
PUBLIC void SysMSleep(uint32_t msecond);

#endif  //_DRIVER_CHAR_CLOCK_H
