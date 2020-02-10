/*
 * file:		include/hal/char/cpu.h
 * auther:		Jason Hu
 * time:		2019/6/27
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _HAL_CPU_H
#define _HAL_CPU_H

#include <lib/stdint.h>
#include <lib/types.h>

/* CMD */
#define CPU_HAL_IO_INVLPG           1
#define CPU_HAL_IO_STEER            2

/* PARAM - CPU_HAL_IO_STEER*/
#define CPU_HAL_VENDOR              1
#define CPU_HAL_BRAND               2
#define CPU_HAL_FAMILY              3
#define CPU_HAL_MODEL               4
#define CPU_HAL_STEEPING            5
#define CPU_HAL_MAX_CPUID           6
#define CPU_HAL_MAX_CPUID_EXT       7
#define CPU_HAL_TYPE                8




PUBLIC void InitCpu();

#endif  //_HAL_CPU_H
