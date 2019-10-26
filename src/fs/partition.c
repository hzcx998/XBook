/*
 * file:		fs/partition.c
 * auther:		Jason Hu
 * time:		2019/9/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <book/share.h>
#include <fs/partition.h>
#include <driver/ide.h>
#include <fs/bofs/bofs.h>
#include <book/device.h>

/* 分区表数，每个磁盘一个分区表 */
struct DiskPartitionTable *dptTable[NR_DPT];

PRIVATE void InitDPTE(struct DiskPartitionTableEntry *dpte,
	unsigned int flags,
	unsigned int startLba,
	unsigned int sectorsLimit)
{
	/* 设置文件类型，absurd文件系统方式 */
	dpte->type = FS_TYPE_ABSURD;
	/* 设置文件类型，可引导 */
	dpte->flags = flags;
	/* 由于不适用CHS模式，所以这里就只填写lba模式信息 */
	dpte->startLBA = startLba;
	dpte->sectorsLimit = sectorsLimit;	
	
	/* 开始和结束都置0 */
	dpte->startHead = dpte->startCylinder = dpte->startSector = 0;
	dpte->endHead = dpte->endCylinder = dpte->endSector = 0;
	/*printk("DPTE: flags %x start lba %d limit sectors %d\n", 
		flags,
		startLba, sectorsLimit);
	*/
}


/**
 * CreateDiskPartition - 创建分区表
 * 
 * 如果分区表不存在，就创建，如果存在，就直接读取返回
 */
PUBLIC int CreateDiskPartition(unsigned int deviceID, struct DiskPartitionTable *dpt)
{
	/*if (DeviceCheckID_OK(deviceID)) {
        printk(PART_ERROR "device %d not exist or not register!\n", deviceID);
        return -1;
    }*/

	printk("create on device %d\n", deviceID);

	/* 读取磁盘分区表，基本都是位于0扇区 */
	if (DeviceRead(deviceID, 0, dpt, 1)) {
		printk(PART_ERROR "device %d read DPT failed!\n", deviceID);
		return -1;
	}

	/* 检查是否已经有分区 */
	if (dpt->BS_TrailSig == BS_TRAIL_SIGN) {
		printk("disk partition has existed.\n");
		/* 已经存在分区表，直接使用就行了 */
		return 0;
	}
	
	/* 创建分区信息 */

	/* 填写引导标志 */
	dpt->BS_TrailSig = BS_TRAIL_SIGN;

	/* 获取磁盘扇区数 */
	unsigned int diskSectors = 0;
    DeviceIoctl(deviceID, 1, (int)&diskSectors);
    printk("device %d sector size is %d\n", deviceID, diskSectors);

	/* 填写第一个分区的信息 */
	unsigned int partitionStart = 2;
	unsigned int partitionSectors = (diskSectors-2)/2;
	InitDPTE(&dpt->DPTE[0], FS_FLAGS_BOOT, partitionStart, partitionSectors);
	
	/* 填写第二个分区的信息 */
	partitionStart += partitionSectors;
	partitionSectors = diskSectors - partitionSectors;
	InitDPTE(&dpt->DPTE[1], 0, partitionStart, partitionSectors);
	
	/* 写回磁盘 */
	if (DeviceWrite(deviceID, 0, dpt, 1)) {
		printk(PART_ERROR "device %d write DPT failed!\n", deviceID);
		return -1;
	}

	return 0;
}

#define DEBUG_MAX_DISK_NR 4

/**
 * InitDiskPartiton - 初始化磁盘分区
 * 
 * 在初始化硬盘驱动结束之后，需要初始化磁盘的分区，
 * 把已经存在的硬盘都进行初始化，然后再在其基础上建立文件系统
 */
PUBLIC void InitDiskPartiton()
{
	int i = 0;
	int deviceID = DEV_HDA;

	if (ideDiskFound > DEBUG_MAX_DISK_NR)
	{
		ideDiskFound = DEBUG_MAX_DISK_NR;
	}
	
	/* 先初始化磁盘分区 */
	for (i = 0; i < ideDiskFound; i++) {
	
		/* 如果没有分区，就创建分区 */
		dptTable[i] = kmalloc(SECTOR_SIZE, GFP_KERNEL);
		if (dptTable[i] == NULL) {
			printk(PART_ERROR "kmalloc for DPT failed!\n");
			return;
		}
		/* 创建分区表（如果已经存在，就只读取分区表） */
		if (CreateDiskPartition(deviceID, dptTable[i])) {
			printk(PART_ERROR "create disk partition failed!\n");
			return;
		}
		/* 指向下一个设备 */
		deviceID++;
	}
	
}