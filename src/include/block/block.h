/*
 * file:		include/block/block.h
 * auther:		Jason Hu
 * time:		2019/10/7
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_BLOCK_H
#define _DRIVER_BLOCK_H

#include <lib/stddef.h>

#define BLOCK_READ 1
#define BLOCK_WRITE 2

/* 一个块有多少个扇区 */
#define BLOCK_SECTORS       2

#define BLOCK_SIZE (SECTOR_SIZE * BLOCK_SECTORS)

PUBLIC void BlockDiskSync();
PUBLIC void InitBlockDevice();

#endif   /* _DRIVER_BLOCK_H */
