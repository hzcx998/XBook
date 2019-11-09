/*
 * file:		fs/node.c
 * auther:		Jason Hu
 * time:		2019/11/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/string.h>
#include <driver/ide.h>
#include <driver/clock.h>
#include <fs/bitmap.h>
#include <share/math.h>
#include <fs/file.h>
#include <book/blk-buffer.h>
#include <fs/directory.h>
#include <fs/super_block.h>

/**
 * SyncSuperBlock - 把超级块同步到磁盘
 * @sb: 超级块
 * 
 */
PUBLIC int SyncSuperBlock(struct SuperBlock *sb)
{
	if (!BlockWrite(sb->devno, sb->superBlockLba, sb, 0)) {
		printk("Sync Super Block Failed!\n");
		return 0;
	}
	return 1;
}

PUBLIC void DumpSuperBlock(struct SuperBlock *sb)
{
	printk(PART_TIP "----Super Block----\n");
	printk(PART_TIP "magic:%x devno:%x total blocks:%d inodes:%d block size:%d inodes in block:%d\n",
		sb->magic, sb->devno, sb->totalBlocks, sb->maxInodes, sb->blockSize, sb->inodeNrInBlock);
	
	printk(PART_TIP "super block lba:%d data start lba:%d\n",
		sb->superBlockLba, sb->dataStartLba);
	
	printk(PART_TIP "block bitmap lba:%d block bitmap blocks:%d\n",
		sb->blockBitmapLba, sb->blockBitmapBlocks);
	
	printk(PART_TIP "node bitmap lba:%d node bitmap blocks:%d\n",
		sb->nodeBitmapLba, sb->nodeBitmapBlocks);
	
	printk(PART_TIP "node table lba:%d node table blocks:%d\n",
		sb->nodeTableLba, sb->nodeTableBlocks);
	
	printk(PART_TIP "block bitmap:%x len:%d node bitmap:%x len:%d\n",
		sb->blockBitmap.bits, sb->blockBitmap.btmpBytesLen, sb->nodeBitmap.bits, sb->nodeBitmap.btmpBytesLen);
	
}
