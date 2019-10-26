/*
 * file:		include/hal/block/ram.h
 * auther:		Jason Hu
 * time:		2019/7/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _HAL_BLOCK_RAM_H
#define _HAL_BLOCK_RAM_H

#include <share/stdint.h>
#include <share/types.h>
#include <share/const.h>

#include <book/hal.h>

//I/O 操作
#define RAM_HAL_IO_MEMSIZE   1

/* 最小物理内存 */
#define RAM_HAL_BASIC_SIZE (128*MB)

#endif  //_HAL_BLOCK_RAM_H
