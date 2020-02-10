/*
 * file:		include/clock/clock.h
 * auther:		Jason Hu
 * time:		2019/6/26
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_CLOCK_H
#define _DRIVER_CLOCK_H

#include <lib/stdint.h>
#include <lib/types.h>

/* 设置默认时钟为pit时钟 */
#include <clock/pit/clock.h>

PUBLIC void InitClockSystem();

#endif  /* _DRIVER_CLOCK_H */
