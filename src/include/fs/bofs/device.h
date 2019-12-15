/*
 * file:		include/fs/bofs/device.h
 * auther:		Jason Hu
 * time:		2019/12/7
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOFS_DEVICE_H
#define _BOFS_DEVICE_H


#include <share/stddef.h>


#include <book/blk-dev.h>

#include <fs/bofs/super_block.h>

PUBLIC int BOFS_MakeDevice(const char *pathname, char type, 
        dev_t devno, size_t size, struct BOFS_SuperBlock *sb);

#endif  /* _BOFS_DEVICE_H */
