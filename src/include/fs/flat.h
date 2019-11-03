/*
 * file:		include/fs/flat.h
 * auther:		Jason Hu
 * time:		2019/10/25
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _FS_FLAT
#define _FS_FLAT

/*
FFS，平坦文件系统（Flat File System）
*/

#include <share/const.h>
#include <share/stddef.h>
#include <book/block.h>
#include <book/device.h>
#include <book/bitmap.h>

/* 路径长度 */
#define FLAT_MAGIC   0x19980325

/* 路径长度 */
#define PATH_NAME_LEN   128

/* 文件名长度 */
#define FILE_NAME_LEN   100

/* 目录名长度，-1是减去':'占用的空间 */
#define DIR_NAME_LEN    (PATH_NAME_LEN - FILE_NAME_LEN - 1)

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


    struct Bitmap blockBitmap;	    /* 块位图 */
	struct Bitmap nodeBitmap;		/* 节点位图 */
	
	unsigned char *iobuf;			/* io时用的缓冲区 */
}__attribute__ ((packed));;



PUBLIC int InitFlatFileSystem();

/* 路径解析 */
PUBLIC int ParseDirectoryName(char *path, char *buf);
PUBLIC int ParseFileName(char *path, char *buf);
PUBLIC struct SuperBlock *BuildFS(dev_t devno,
	sector_t start,
	sector_t count,
	size_t blockSize,
	size_t nodeNr);
PUBLIC void DumpSuperBlock(struct SuperBlock *sb);
PUBLIC int SyncSuperBlock(struct SuperBlock *sb);

#endif  /* _FS_FLAT */
