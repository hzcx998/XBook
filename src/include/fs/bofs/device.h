/*
 * file:		include/fs/bofs/device.h
 * auther:		Jason Hu
 * time:		2019/9/18
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _FS_BOFS_DEVICE_H
#define _FS_BOFS_DEVICE_H

#include <share/types.h>
#include <share/stdint.h>

int BOFS_MakeDevice(const char *path, char type, int deviceID);

#endif /* _FS_BOFS_DEVICE_H */