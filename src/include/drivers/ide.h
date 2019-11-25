/*
 * file:		include/driver/ide.h
 * auther:		Jason Hu
 * time:		2019/10/13
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_IDE_H
#define _DRIVER_IDE_H

#include <share/stdint.h>
#include <share/types.h>
#include <share/const.h>

/* IO命令 */
enum {
	IDE_IO_CLEAN = 1,
    IDE_IO_SECTORS,
    IDE_IO_BLKZE,
	IDE_IO_NR,
};

EXTERN unsigned char ideDiskFound;


PUBLIC int InitIdeDriver();
PUBLIC void ExitIdeDriver();

#endif  /* _DRIVER_IDE_H */
