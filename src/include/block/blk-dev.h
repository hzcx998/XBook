/*
 * file:		include/block/blk-dev.h
 * auther:		Jason Hu
 * time:		2019/10/13
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BLOCK_DEVICE_H
#define _BLOCK_DEVICE_H

#include <lib/types.h>
#include <book/atomic.h>
#include <book/list.h>
#include <book/device.h>
#include <block/blk-disk.h>

/**
 * 块设备是每一个磁盘或者分区都应该有的一个结构
 * 是一个抽象集合 
 */
struct BlockDevice {
    struct Device super;        /* 继承设备抽象 */

    struct List list;   /* 连接所有块设备的链表 */

    /* 块设备容器
     * block device 既可以是disk的抽象，又可以是disk partition的抽象
     * 当作为分区的抽象时，contians指向了该分区所属的disk对应的block device
     * 当作为disk抽象时，contiaons指向了自身的block device
     */
    struct BlockDevice *contains;

    unsigned int blockSize;     /* 每个块的大小 */
    struct DiskPartition *part; /* 指向分区指针，对于disk，指向内置的分区0 */

    /* 置1表示内存中的分区信息无效，下次打开设备时需要重新扫描分区 */
    int invalided;

    /**
     * 磁盘抽象，当block device 作为分区抽象时，指向该分区所属的disk，
     * 当作为disk抽象时，指向自身
     */
    struct Disk *disk;

    void *private;  /* 私有数据 */
};



#define SIZEOF_BLOCK_DEVICE sizeof(struct BlockDevice)

PUBLIC struct BlockDevice *AllocBlockDevice(dev_t devno);
PUBLIC void BlockDeviceInit(struct BlockDevice *blkdev,
    struct Disk *disk,
    int partno,
    int blockSize,
    void *private);
PUBLIC void BlockDeviceSetup(struct BlockDevice *blkdev,
    struct DeviceOperations *opSets);
PUBLIC void AddBlockDevice(struct BlockDevice *blkdev);
PUBLIC void DelBlockDevice(struct BlockDevice *blkdev);
PUBLIC void DumpBlockDevice(struct BlockDevice *blkdev);
PUBLIC void BlockDeviceSetName(struct BlockDevice *blkdev, char *name);
PUBLIC struct BlockDevice *GetBlockDeviceByDevno(dev_t devno);
PUBLIC struct BlockDevice *GetBlockDeviceByName(char *name);



#endif   /* _BLOCK_DEVICE_H */
