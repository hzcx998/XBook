/*
 * file:		drivers/ramdisk.c
 * auther:		Jason Hu
 * time:		2019/9/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <book/arch.h>
#include <book/config.h>
#include <book/debug.h>
#include <book/interrupt.h>
#include <drivers/ramdisk.h>
#include <book/ioqueue.h>
#include <book/device.h>
#include <share/string.h>
#include <book/task.h>
#include <share/math.h>
#include <share/const.h>
#include <book/vmarea.h>
#include <book/block.h>
#include <share/vsprintf.h>

#include <book/blk-request.h>
#include <book/blk-disk.h>
#include <book/blk-dev.h>
#include <book/blk-buffer.h>

/* 配置开始 */
//#define _DEBUG_RAMDISK
/* 配置结束 */

#define MAX_SECTORS			10240

#define MAX_RAMDISK_NR			1


PRIVATE struct RamdiskDevice {
	struct Disk *disk;
	struct RequestQueue *requestQueue;
	unsigned char *data;
	unsigned int size;
}devices[MAX_RAMDISK_NR];

/**
 * RamdiskReadSector - 读扇区
 * @dev: 设备
 * @lba: 逻辑扇区地址
 * @buf: 扇区缓冲
 * @count: 扇区数
 * 
 * 数据读取磁盘，成功返回0，失败返回-1
 */
PRIVATE int RamdiskReadSector(struct RamdiskDevice *dev,
	unsigned int lba,
	void *buf,
	unsigned int count)
{
	/* 检查设备是否正确 */
	if (dev < devices || dev >= &devices[MAX_RAMDISK_NR]) {
		return -1;
	} else if (lba + count > dev->size) {
		return -1;
	} else {
		/* 进行磁盘访问 */
		memcpy(buf, dev->data + lba * SECTOR_SIZE, count * SECTOR_SIZE);
	}
	return 0;
}

/**
 * RamdiskWriteSector - 写扇区
 * @dev: 设备
 * @lba: 逻辑扇区地址
 * @buf: 扇区缓冲
 * @count: 扇区数
 * 
 * 数据读取磁盘，成功返回0，失败返回-1
 */
PRIVATE int RamdiskWriteSector(struct RamdiskDevice *dev,
	unsigned int lba,
	void *buf,
	unsigned int count)
{
	/* 检查设备是否正确 */
	if (dev < devices || dev >= &devices[MAX_RAMDISK_NR]) {
		return -1;
	} else if (lba + count > dev->size) {
		return -1;
	} else {
		/* 进行磁盘访问 */
		memcpy(dev->data + lba * SECTOR_SIZE, buf, count * SECTOR_SIZE);
	}
	return 0;
}
/**
 * DoBlockRequest - 执行请求
 * @q: 请求队列
 * 
 * 这是文件系统块设备的接口，它和文件系统相关
 */
PRIVATE void DoBlockRequest(struct RequestQueue *q)
{
	struct Request *rq;
	struct RamdiskDevice *dev = q->queuedata;

	//printk("DoBlockRequest: start\n");

	rq = BlockFetchRequest(q);
	while (rq != NULL)
	{
		#ifdef _DEBUG_RAMDISK
		printk("dev %s: ", rq->disk->diskName);
		printk("queue %s waiter %s cmd %s disk lba %d\n",
			q->requestList == &q->upRequestList ? "up" : "down", rq->waiter->name, rq->cmd == BLOCK_READ ? "read" : "write", rq->lba);
		#endif

		/* 执行单个扇区操作 */
		if (rq->cmd == BLOCK_READ) {
			RamdiskReadSector(dev, rq->lba, rq->buffer, rq->count);
		} else {
			RamdiskWriteSector(dev, rq->lba, rq->buffer, rq->count);
		}

		/* 结束当前请求 */
		BlockEndRequest(rq, 0);

		/* 获取一个新情求 */
		rq = BlockFetchRequest(q);
	}
	//printk("DoBlockRequest: end\n");
}


/**
 * IdeCleanDisk - 清空磁盘的数据
 * @dev: 设备
 * @count: 清空的扇区数
 * 
 * 当count为0时，表示清空整个磁盘，不为0时就清空对于的扇区数
 */
PRIVATE int RamdiskCleanDisk(struct RamdiskDevice *dev, sector_t count)
{
	if (count == 0)
		count = dev->size;

	sector_t todo, done = 0;
	
	/* 每次写入10个扇区 */
	char *buffer = kmalloc(SECTOR_SIZE *10, GFP_KERNEL);
	if (!buffer) {
		printk("kmalloc for Ramdisk buf failed!\n");
		return -1;
	}

	memset(buffer, 0, SECTOR_SIZE *10);

	printk(PART_TIP "Ramdisk clean: count%d\n", count);
	while (done < count) {
		/* 获取要去操作的扇区数这里用10作为分界 */
		if ((done + 10) <= count) {
			todo = 10;
		} else {
			todo = count - done;
		}
		RamdiskWriteSector(dev, done, buffer, todo);
		done += 10;
	}
	return 0;
}


/**
 * 编写设备操作接口
 */

/**
 * KeyboardIoctl - 键盘的IO控制
 * @device: 设备项
 * @cmd: 命令
 * @arg1: 参数1
 * @arg2: 参数2
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int RamdiskIoctl(struct Device *device, int cmd, int arg)
{
	int retval = 0;

	/* 获取Ramdisk设备 */
	struct BlockDevice *blkdev = (struct BlockDevice *)device;
	struct RamdiskDevice *dev = (struct RamdiskDevice *)blkdev->private;
	
	switch (cmd)
	{
	case RAMDISK_IO_CLEAN:	/* 执行清空磁盘命令 */
		if (RamdiskCleanDisk(dev, arg)) {
			retval = -1;
		}
		break;
    case RAMDISK_IO_SECTORS:	/* 获取扇区数 */
        *((sector_t *)arg) = blkdev->part->sectorCounts;

		break;
    case RAMDISK_IO_BLKZE:	/* 获取块大小 */
        *((sector_t *)arg) = blkdev->blockSize;
		break;
	default:
		/* 失败 */
		retval = -1;
		break;
	}

	return retval;
}

/**
 * RamdiskRead - 读取数据
 * @device: 设备
 * @lba: 逻辑扇区地址
 * @buffer: 缓冲区
 * @count: 扇区数
 * 
 * @return: 成功返回0，失败返回-1
 */
PRIVATE int RamdiskRead(struct Device *device, unsigned int lba, void *buffer, unsigned int count)
{
	/* 获取Ramdisk设备 */
	struct BlockDevice *blkdev = (struct BlockDevice *)device;
	struct RamdiskDevice *dev = (struct RamdiskDevice *)blkdev->private;
	
	return RamdiskReadSector(dev, lba, buffer, count);
}

/**
 * RamdiskWrite - 写入数据
 * @device: 设备
 * @lba: 逻辑扇区地址
 * @buffer: 缓冲区
 * @count: 扇区数
 * 
 * @return: 成功返回0，失败返回-1
 */
PRIVATE int RamdiskWrite(struct Device *device, unsigned int lba, void *buffer, unsigned int count)
{
	/* 获取IDE设备 */
	struct BlockDevice *blkdev = (struct BlockDevice *)device;
	struct RamdiskDevice *dev = (struct RamdiskDevice *)blkdev->private;
	
	return RamdiskWriteSector(dev, lba, buffer, count);
}

/**
 * 块设备操作的结构体
 */
PRIVATE struct DeviceOperations opSets = {
	.ioctl = RamdiskIoctl,
	.read = RamdiskRead,
	.write = RamdiskWrite,
};

/**
 * RamdiskCreateDevice - 创建块设备
 * @dev: 磁盘设备
 * @major: 主设备号
 * @diskIdx: 磁盘的索引
 */
PRIVATE int RamdiskCreateDevice(struct RamdiskDevice *dev, int major, int idx)
{
	dev->disk = AllocDisk(1);
	if (dev->disk == NULL)
		return -1;
	
	dev->requestQueue = BlockInitQueue(DoBlockRequest, dev);
	if (dev->requestQueue == NULL) {
		return -1;
	}
	
	/* 设置磁盘相关 */
	char name[DISK_NAME_LEN];
	char devname;
	/* 选择磁盘名 */
	switch (idx)
	{
	case 0:
		devname = 'a';
		break;
	case 1:
		devname = 'b';
		break;
	default:
		break;
	}

	sprintf(name, "rd%c", devname);
	DiskIdentity(dev->disk, name, major, RAMDISK_MINOR(idx));
	DiskBind(dev->disk, dev->requestQueue, dev);
	dev->size = MAX_SECTORS;
	SetCapacity(dev->disk, dev->size);
	
	AddDisk(dev->disk);

	/* 添加一个Disk块设备 */
	struct BlockDevice *blkdev = AllocBlockDevice(MKDEV(major, dev->disk->firstMinor));
	if (blkdev == NULL) {
		return -1;
	}
	BlockDeviceInit(blkdev, dev->disk, -1, SECTOR_SIZE * 2, dev);
	BlockDeviceSetup(blkdev, &opSets);
	BlockDeviceSetName(blkdev, name);
	
	AddBlockDevice(blkdev);

	/* 添加一个分区块设备 */
	/*int i;
	for (i = 0; i < 2; i++) {
		// + 1 是因为在此之前为disk设置了一个block device 
		blkdev = AllocBlockDevice(MKDEV(major, dev->disk->firstMinor + i + 1));
		if (blkdev == NULL) {
			return -1;
		}
		// 添加分区
		AddPartition(dev->disk, i, i * 512, 512);
		
		BlockDeviceInit(blkdev, dev->disk, i, SECTOR_SIZE * 2, dev);

		AddBlockDevice(blkdev);
	}*/
	return 0;
}

/**
 * RamdiskDelDevice - 删除一个块设备
 * @dev: 要删除的块设备
 */
void RamdiskDelDevice(struct RamdiskDevice *dev)
{
	if (dev->disk)
		DelDisk(dev->disk);
	if (dev->requestQueue)
		BlockCleanUpQueue(dev->requestQueue);
}

/**
 * InitRamdiskDriver - 初始化ramdisk磁盘驱动
 */
PUBLIC int InitRamdiskDriver()
{
	PART_START("Ramdisk Driver");

	int status;

	/* 创建一个块设备 */
	int i;
	for (i = 0; i < MAX_RAMDISK_NR; i++) {
		status = RamdiskCreateDevice(&devices[i], RAMDISK_MAJOR, i);
		if (status < 0)
			return status;

		/* 磁盘数据 */
		devices[i].data = vmalloc(devices[i].disk->capacity * SECTOR_SIZE);
		if (devices[i].data == NULL) {
			printk("vmalloc for ramdisk failed!\n");
			return -1;
		}
		memset(devices[i].data, i+1, devices[i].disk->capacity * SECTOR_SIZE);
	}

    PART_END();
	return 0;
}

/**
 * ExitRamdiskdDriver - 退出驱动
 */
PUBLIC void ExitRamdiskdDriver()
{
	int i;
	for (i = 0; i < 2; i++) {
		RamdiskDelDevice(&devices[i]);
		vfree(devices[i].data);
	}
	
}
