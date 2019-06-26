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

PUBLIC void ClockInit();

#endif  //_DRIVER_CHAR_CLOCK_H
