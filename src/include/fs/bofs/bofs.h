/*
 * file:		include/fs/bofs/bofs.h
 * auther:		Jason Hu
 * time:		2019/9/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _FS_BOFS_H
#define _FS_BOFS_H

/*
Book File System (BOFS) -V0.2
这是对v0.1的升级。
0.2主要用于X内核
*/

#include <share/types.h>
#include <share/stdint.h>
#include <share/const.h>

#include <fs/partition.h>

#include <fs/bofs/super_block.h>

/*
-----
super block
-----
sector bitmap
-----
inode bitmap
-----
inode table
-----
data
-----
*/
//extern struct bofs_super_block *bofs_current_super_block;

/*bofs basic*/

PUBLIC int BOFS_Init();


PUBLIC void BOFS_Test();

PUBLIC int BOFS_MountMasterFS(struct BOFS_SuperBlock *superBlock);
PUBLIC struct BOFS_SuperBlock *BOFS_MakeFS(struct DiskPartitionTableEntry *dpte,
    unsigned int deviceID);
void BOFS_Format(int deviceID, uint32 startSector, uint32 totalSector, uint32 inodeNr);

#endif

