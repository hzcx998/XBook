/*
 * file:		include/fs/super_block.h
 * auther:		Jason Hu
 * time:		2019/10/23
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _FS_SUPER_BLOCK
#define _FS_SUPER_BLOCK

#include <share/stdint.h>
#include <share/types.h>
#include <share/string.h>


struct SuperBlock {
	unsigned int magic;     /* 魔数，用于标记是否已经安装了文件系统 */
	dev_t devno;		    /* 所在的磁盘的设备号 */
	sector_t totalBlocks;	/* 这个文件系统占用多少块 */
	
	sector_t superBlockLba;   /* 超级块的起始块 */
	
	sector_t blockBitmapLba;	/* 块位图起始地址 */
	sector_t blockBitmapBlocks;	/* 块位图块数 */
	
	sector_t nodeBitmapLba;	    /* 节点文件位图起始地址 */
	sector_t nodeBitmapBlocks;	/* 节点文件位图块数 */
	
	sector_t nodeTableLba;	    /* 节点表起始位置 */
	sector_t nodeTableBlocks;	/* 节点表占用块数 */
	
	sector_t dataStartLba;      /* 数据区域起始地址 */
	
	char inodeNrInBlock;	    /* 一个扇区可以容纳多少个节点 */
	
	size_t maxInodes;	    	/* 最多有多少个节点 */
    size_t blockSize;	    	/* 一个块的大小 */
    
	size_t files;				/* 文件数 */
	size_t highestFiles;		/* 记录最高的文件数 */

    struct Bitmap blockBitmap;	    /* 块位图 */
	struct Bitmap nodeBitmap;		/* 节点位图 */
	
	unsigned char *iobuf;			/* io时用的缓冲区 */
}__attribute__ ((packed));;

PUBLIC void DumpSuperBlock(struct SuperBlock *sb);
PUBLIC int SyncSuperBlock(struct SuperBlock *sb);

#endif  /* _FLAT_SUPER_BLOCK */
