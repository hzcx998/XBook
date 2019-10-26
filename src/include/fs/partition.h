/*
 * file:		include/fs/partition.h
 * auther:		Jason Hu
 * time:		2019/9/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

/*
磁盘分区结构描述，用于对磁盘进行分区管理
*/

#ifndef _FS_PARTITION
#define _FS_PARTITION

#include <share/stdint.h>
#include <share/types.h>
#include <share/string.h>

/* 每个分区表有4个分区表项 */
#define NR_DPTE     4

/* 每个磁盘有1个分区表，这里每个磁盘设备都有一个磁盘分区表 */
#define NR_DPT      (4 + 0)

#define BS_TRAIL_SIGN     0xaa55

/* absurd的文件类型约定位0x90，用fdisk产看，0x90是空闲，所以我们使用它 */
#define FS_TYPE_ABSURD     0x90

/* 文件系统活动标志，0x80表示可以引导，0x00表示不可以 */
#define FS_FLAGS_BOOT     0x80

/* 磁盘分区表项 */
struct DiskPartitionTableEntry {
    /* 活动标志位，0x80表示可引导，0表示不可以，其他值非法 */
	unsigned char flags;

    /* 分区开始位置（CHS） */
	unsigned char startHead;
	unsigned short  startSector	:6,	//0~5
			startCylinder	:10;	//6~15

    /* 文件系统类型，0表示不可识别的文件系统 */
	unsigned char type;

    /* 分区结束位置（CHS） */
	unsigned char endHead;
	unsigned short  endSector	:6,	//0~5
			endCylinder	:10;	    //6~15

    /* 分区开始位置（LBA） */
	unsigned int startLBA;
    /* 分区占用扇区数（LBAS） */
	unsigned int sectorsLimit;
}__attribute__((packed));

/* 磁盘分区表 */
struct DiskPartitionTable {
    /* 引导代码，保留 */
	unsigned char BS_Reserved[446];
    /* 分区表信息 */
	struct DiskPartitionTableEntry DPTE[NR_DPTE];
	
    /* 引导标志 */
    unsigned short BS_TrailSig;
}__attribute__((packed));

extern struct DiskPartitionTable *dptTable[];

PUBLIC void InitDiskPartiton();

#endif  //_FS_PARTITION
