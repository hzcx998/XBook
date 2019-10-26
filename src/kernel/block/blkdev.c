/*
 * file:	    kernel/block/blkdev.c
 * auther:	    Jason Hu
 * time:	    2019/10/13
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/debug.h>
#include <share/string.h>
#include <book/block.h>

#include <book/blk-dev.h>

EXTERN struct List AllDeviceListHead;

/* 所有块设备的队列 */
LIST_HEAD(allBlockDeviceList);

/**
 * GetBlockDeviceByDevno - 通过设备号获取块设备
 * @devno: 设备号
 * 
 * 返回块设备
 */
PUBLIC struct BlockDevice *GetBlockDeviceByDevno(dev_t devno)
{
    struct BlockDevice *device;

    ListForEachOwner(device, &allBlockDeviceList, list) {
        if (device->super.devno == devno) {
            return device;
        }
    }
    return NULL;
}

PUBLIC void DumpBlockDevice(struct BlockDevice *blkdev)
{
    printk(PART_TIP "----Block Device----\n");

    printk(PART_TIP "devno:%x contains:%x part:%x disk:%x \n",
        blkdev->super.devno, blkdev->contains, blkdev->part, blkdev->disk);
    
    printk(PART_TIP "name:%s blockSize:%d invalided:%x private:%x\n",
        blkdev->super.name, blkdev->blockSize, blkdev->invalided, blkdev->private);
}

/**
 * AllocBlockDevice - 分配一个块设备
 * @devno: 表示块设备号
 * 
 * 分配一个和块设备号对应的块设备
 */
PUBLIC struct BlockDevice *AllocBlockDevice(dev_t devno)
{
    struct BlockDevice *device = kmalloc(SIZEOF_BLOCK_DEVICE, GFP_KERNEL);
    if (device == NULL)
        return NULL;
    
    device->super.devno = devno;
    device->super.type = DEV_TYPE_BLOCK;
    
    return device;
}

/**
 * BlockDeviceInit - 对一个块设备进行初始化
 * @blkdev: 块设备
 * @disk: 块设备对应的磁盘
 * @partno: 块设备对应的分区号。
 *      （为-1表示没有分区，即描述的是磁盘。
 *         >=0表示分区号，描述的是分区）
 * @blockSize:单个块的大小（扇区大小的倍数，512，1024，2048，4096（MAX））
 * @private: 私有数据，创建者可以使用的区域
 */
PUBLIC void BlockDeviceInit(struct BlockDevice *blkdev,
    struct Disk *disk,
    int partno,
    int blockSize,
    void *private)
{
    if (partno >= disk->minors) {
        partno = disk->minors - 1;
    }

    /* 如果不指定part，那么就说嘛是描述Disk的，也就是说contains指向自己 */
    if (partno == -1) {
        blkdev->contains = blkdev;
        disk->blkdev = blkdev;

        /* 指向第一个分区 */
        blkdev->part = &disk->part0;

    } else {
        blkdev->contains = disk->blkdev;

        blkdev->part = &disk->part[partno];
    }

    blkdev->invalided = 0;
    blkdev->disk = disk;

    blkdev->blockSize = blockSize;
    blkdev->private = private;

    /* 设备的私有数据就是块设备的地址 */
    blkdev->super.private = blkdev;
}

/**
 * BlockDeviceSetup - 对一个字符设备进行调整
 * @blkdev: 块设备
 * @opSets: 操作集
 * 
 */
PUBLIC void BlockDeviceSetup(struct BlockDevice *blkdev,
    struct DeviceOperations *opSets)
{
    blkdev->super.opSets = opSets;
}


/**
 * AddBlockDevice - 把块设备添加到内核
 * @blkdev: 要添加的块设备
 */
PUBLIC void AddBlockDevice(struct BlockDevice *blkdev)
{
    /* 添加到队列的最后面 */
    ListAddTail(&blkdev->list, &allBlockDeviceList);

    /* 添加到设备队列的最后面 */
    ListAddTail(&blkdev->super.list, &AllDeviceListHead);

    return;
}

/**
 * DelBlockDevice - 删除一个块设备
 * @blkdev: 块设备
 * 
 * 删除块设备，就是从内核中除去对它的描述
 */
PUBLIC void DelBlockDevice(struct BlockDevice *blkdev)
{
    /* 如果有是描述磁盘的块设备，就要把对应得指针置空 */
    if (blkdev->disk->blkdev == blkdev) {
        blkdev->disk->blkdev = NULL;
    }
    blkdev->part = NULL;

    ListDel(&blkdev->list);

    ListDel(&blkdev->super.list);
}

/**
 * FreeBlockDevice - 释放一个块设备
 * @blkdev: 块设备
 * 
 * 释放块符设备占用的内存
 */
PUBLIC void FreeBlockDevice(struct BlockDevice *blkdev)
{
    if (blkdev) 
        kfree(blkdev);
}

/**
 * BlockDeviceSetName - 设置块设备的名字
 * @blkdev: 块设备
 * @name: 名字
 * 
 */
PUBLIC void BlockDeviceSetName(struct BlockDevice *blkdev, char *name)
{
    memset(blkdev->super.name, 0, DEVICE_NAME_LEN);
    strcpy(blkdev->super.name, name);
}

