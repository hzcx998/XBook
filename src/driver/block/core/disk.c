/*
 * file:	    block/core/disk.c
 * auther:	    Jason Hu
 * time:	    2019/10/13
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/debug.h>
#include <lib/string.h>

#include <block/block.h>
#include <block/blk-disk.h>

/* 所有磁盘的队列 */
LIST_HEAD(allDiskList);

/**
 * AllocDisk - 分配一块磁盘
 * @minors: 该磁盘可以有多少个子分区
 * 
 * 分配一个磁盘，并返回磁盘结构，失败返回空
 */
PUBLIC struct Disk *AllocDisk(int minors)
{
    struct Disk *disk = kmalloc(SIZEOF_DISK, GFP_KERNEL);
    if (disk == NULL)
    {
        return NULL;
    }
    
    if (minors < 0)
        minors = 0;

    /* 初始化基础信息 */
    disk->minors = minors;

    if (disk->minors > 0) {        
        /* 根据minors创建对应的分区指针 */
        struct DiskPartition *part = kmalloc(sizeof(struct DiskPartition) * minors, GFP_KERNEL);
        if (disk == NULL)
        {
            kfree(disk);
            return NULL;
        }
        /* 指向分区表 */
        disk->part = part;
    }

    return disk;
}

/**
 * AddPartition - 添加一个分区到磁盘
 * @disk: 分区所在的磁盘
 * @partno: 分区在磁盘中的磁盘号
 * @start: 分区开始位置
 * @count: 分区扇区数
*/
PUBLIC void AddPartition(struct Disk *disk, int partno, sector_t start, sector_t count)
{
    /* 分区号不在范围内 */
    if (partno < 0 || partno >= disk->minors) {
        return;
    }
    struct DiskPartition *part = &disk->part[partno]; 

    part->partno = partno;
    part->startSector = start;
    part->sectorCounts = count;
    part->disk = disk;
    
    AtomicSet(&part->ref, 0);
}

/**
 * AddDisk - 添加一个磁盘到内核
 * @disk: 要添加的磁盘
 * 
 * 把一个磁盘注册到内核
 */
PUBLIC void AddDisk(struct Disk *disk)
{
    /* 初始化0分区 */
    disk->part0.partno = 0;
    disk->part0.startSector = 0;
    disk->part0.sectorCounts = disk->capacity;
    AtomicSet(&disk->part0.ref, 0);
    
    /* 初始化缓冲头链表 */
    INIT_LIST_HEAD(&disk->bufferHeadList);

    /* 添加到系统 */
    ListAddTail(&disk->list, &allDiskList);
}

/**
 * SetCapacity - 设置磁盘的大小（扇区为单位）
 * @disk: 磁盘
 * @capacity: 磁盘扇区数
 */
PUBLIC void SetCapacity(struct Disk *disk, size_t capacity)
{
    disk->capacity = capacity;
}

/**
 * DiskIdentity - 设置磁盘基础信息
 * @disk: 磁盘
 * @name: 磁盘名
 * @major: 磁盘主设备号
 * @minor: 磁盘次设备号
 */
PUBLIC void DiskIdentity(struct Disk *disk,
    char *name,
    int major,
    int minor)
{
    memset(disk->diskName, 0, DISK_NAME_LEN);
	strcpy(disk->diskName, name);
    disk->major = major;
    disk->firstMinor = minor;
}

/**
 * DiskBind - 磁盘信息绑定
 * @disk: 磁盘
 * @queue: 请求队列
 * @bdops: 块设备操作
 * @private: 磁盘对应的私有数据，创建者可以使用这个区域
 */
PUBLIC void DiskBind(struct Disk *disk,
    struct RequestQueue *queue,
    void *private)
{
    disk->requestQueue = queue;
    disk->privateData = private;
}

/**
 * DelDisk - 删除一个磁盘
 * @disk: 要删除的磁盘
 * 
 * 从内核中把磁盘和分区删除
 */
PUBLIC void DelDisk(struct Disk *disk)
{
    /* 删除分区 */
    kfree(disk->part);

    /* 删除链接 */
    ListDel(&disk->list);
}

/**
 * GetDiskByName - 通过磁盘名字获取磁盘
 * @name: 磁盘名
 * 
 * 返回磁盘
 */
PUBLIC struct Disk *GetDiskByName(char *name)
{
    struct Disk *disk;

    ListForEachOwner(disk, &allDiskList, list) {
        /* 名字一样就说明找到了该磁盘 */
        if (!strcmp(disk->diskName, name)) {
            return disk;
        }
    }
    return NULL;
}

PUBLIC void DumpDisk(struct Disk *disk)
{
    printk(PART_TIP "----Disk----\n");

    printk(PART_TIP "devno:%x major:%x firstMinor:%d minors:%d name:%s \n",
        MKDEV(disk->major, disk->firstMinor), disk->major, disk->firstMinor, disk->minors, disk->diskName);
    
    printk(PART_TIP "part:%x requestQueue:%x private:%x \n",
        disk->part, disk->requestQueue, disk->privateData);
    
    printk(PART_TIP "capacity:%d blkdev:%x flags:%x\n",
        disk->capacity, disk->blkdev, disk->flags);
    
}

PUBLIC void DumpDiskPartition(struct DiskPartition *part)
{
    printk(PART_TIP "----Disk Partiton----\n");

    printk(PART_TIP "start:%d count:%d partno:%d ref:%d\n",
        part->startSector, part->sectorCounts, part->partno, part->ref);
    
}
