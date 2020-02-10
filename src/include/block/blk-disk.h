/*
 * file:		include/block/blk-disk.h
 * auther:		Jason Hu
 * time:		2019/10/13
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BLOCK_DISK_H
#define _BLOCK_DISK_H

#include <lib/types.h>
#include <book/atomic.h>
#include <book/list.h>

#include <block/blk-request.h>
#include <block/blk-dev.h>

#define DISK_NAME_LEN 32

struct DiskPartition {
    sector_t startSector;       /* 分区的起始扇区号 */
    sector_t sectorCounts;      /* 分区的扇区数，也就是分区容量 */

    int partno; /* 分区号 */
    
    struct Disk *disk;
    Atomic_t ref;       /* 引用计数 */

    void *fsi;  /* file system info, 文件系统信息 */
};

#define SIZEOF_PARTITION sizeof(struct DiskPartition)

/**
 * 磁盘是对每一块物理磁盘设备的描述
 * 
 */
struct Disk {
    struct List list;
    int major;      /* 主设备号 */
    int firstMinor; /* 第一个从设备号 */
    int minors;     /* 从设备数量，如果是1，就不能进行分区 */
    char diskName[DISK_NAME_LEN];  /* 磁盘名称 */

    struct DiskPartition part0;     /* 当没有分区时，默认初始化的分区 */
    struct DiskPartition *part;    /* 磁盘分区 */
    
    //struct BlockDeviceOperations    *bdops; /* 设备操作 */
    struct RequestQueue *requestQueue;   // 磁盘的请求队列
    void *privateData;      /* 私有数据 */
    sector_t capacity;      /* 磁盘扇区数 */
    struct BlockDevice *blkdev; /* 磁盘对应的块设备 */
    int flags;       
    struct List bufferHeadList;   /* 缓冲头链表 */
};

#define SIZEOF_DISK     sizeof(struct Disk)

void AddPartition(struct Disk *disk, int partno, sector_t start, sector_t count);

/* 设置磁盘大小 */
void SetCapacity(struct Disk *disk, size_t capacity);

void DiskIdentity(struct Disk *disk,
    char *name,
    int major,
    int minor);

void DiskBind(struct Disk *disk,
    struct RequestQueue *rq,
    void *private);

/* 分配disk */
struct Disk *AllocDisk(int minnors);

/* 删除disk */
void DelDisk(struct Disk *disk);

/* 增加disk */
void AddDisk(struct Disk *disk);

void DumpDisk(struct Disk *disk);
PUBLIC void DumpDiskPartition(struct DiskPartition *part);

PUBLIC struct Disk *GetDiskByName(char *name);

#endif   /* _BLOCK_DISK_H */
