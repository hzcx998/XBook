/*
 * file:		include/fs/absurd.h
 * auther:		Jason Hu
 * time:		2019/9/1
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _FS_ABSURD
#define _FS_ABSURD

#include <share/stdint.h>
#include <share/types.h>
#include <share/string.h>
#include <fs/partition.h>

PUBLIC int InstallAbsurdFS(struct DiskPartitionTableEntry *dpte,
    unsigned int deviceID);

PUBLIC int InitAbsurdFS();




#endif  //_FS_ABSURD
