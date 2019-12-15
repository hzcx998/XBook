/*
 * file:		fs/bofs/main.c
 * auther:		Jason Hu
 * time:		2019/9/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/string.h>
#include <drivers/ide.h>
#include <share/vsprintf.h>
#include <share/const.h>
#include <book/blk-buffer.h>
#include <book/blk-dev.h>

#include <book/device.h>

#include <fs/bofs/super_block.h>
#include <fs/bofs/inode.h>
#include <fs/bofs/dir_entry.h>
#include <fs/bofs/bitmap.h>
#include <fs/bofs/dir.h>
#include <fs/bofs/file.h>
#include <fs/bofs/drive.h>

EXTERN struct List allBlockDeviceList;

PRIVATE void BOFS_Test()
{

    return;
}

/**
 * BOFS_ProbeFileSystem - 探测分区是否已经有文件系统
 * @part: 分区
 * 
 * 成功返回文件系统超级块指针
 * 失败返回NULL
 */
PRIVATE struct BOFS_SuperBlock *BOFS_ProbeFileSystem(struct BlockDevice *blkdev)
{
    struct Disk *disk = blkdev->disk;
    
    //DumpDisk(disk);

    struct BOFS_SuperBlock *superBlock = kmalloc(blkdev->blockSize, GFP_KERNEL);
    if (superBlock == NULL) {
        printk("kmalloc for superBlock Failed!\n");
        return NULL;
    }
    
    /* 读取超级块 */
    if (BlockRead(MKDEV(disk->major, disk->firstMinor), 1, superBlock)) {
        Panic("block read failed!\n");
    }

    if (BOFS_HAD_FS(superBlock)) {
        return superBlock;
    } else {
        return NULL;
    }
}

/**
 * BOFS_MakeFileSystem - 在分区上创建文件系统
 * @part: 分区
 * 
 * 成功返回文件系统超级块指针
 * 失败返回NULL
 */
PRIVATE struct BOFS_SuperBlock *BOFS_MakeFileSystem(struct BlockDevice *blkdev)
{
    struct Disk *disk = blkdev->disk;
    struct DiskPartition *part = blkdev->part;
    
    struct BOFS_SuperBlock *superBlock = kmalloc(blkdev->blockSize, GFP_KERNEL);
    if (superBlock == NULL) {
        printk("kmalloc for superBlock Failed!\n");
        return NULL;
    }

    if (BOFS_MakeFS(superBlock,
        MKDEV(disk->major, disk->firstMinor), 
        part->startSector,
        part->sectorCounts,
        blkdev->blockSize,
        DEFAULT_MAX_INODE_NR)) 
    {
        printk(PART_WARRING "make book file system on disk failed!\n");
        return NULL;
    }
    return superBlock;
}

/**
 * BOFS_MountFileSystem - 挂载文件系统到内存
 * @superBlock: 超级块
 * 
 * 成功返回文件系统超级块指针
 * 失败返回NULL
 */
PRIVATE int BOFS_MountFileSystem(struct BOFS_SuperBlock *sb)
{
    if (BOFS_MountFS(sb)) {
        printk(PART_ERROR "mount bofs failed!\n");
        return -1;
    }
    
    if (BOFS_OpenRootDir(sb)) {
        printk(PART_ERROR "open dir for bofs failed!\n");
        return -1;
    }

    return 0;
}

/**
 * BOFS_MountFileSystem - 挂载文件系统到内存
 * @superBlock: 超级块
 * 
 * 成功返回文件系统超级块指针
 * 失败返回NULL
 */
PRIVATE int BOFS_UnmountFileSystem(struct BOFS_SuperBlock *sb)
{
    BOFS_CloseDir(sb->rootDir);

    if (BOFS_UnmountFS(sb)) {
        printk(PART_ERROR "mount bofs failed!\n");
        return -1;
    }
    
    return 0;
}

PUBLIC int InitBoFS()
{
    PART_START("BOFS");

    BOFS_InitFdTable();

    /* 获取设备文件 */
    struct BlockDevice *device;

    struct BOFS_SuperBlock *sb;

    char driveName[BOFS_DRIVE_NAME_LEN];
    /* 根据设备来创建一个文件系统，如果已经存在，直接加载即可 */
    ListForEachOwner(device, &allBlockDeviceList, list) {
        //printk("device:%s\n", device->super.name);

        /* 探测是否有文件系统 */
        sb = BOFS_ProbeFileSystem(device);
        
        if (sb == NULL) { /* 没有文件系统 */
            sb = BOFS_MakeFileSystem(device);
            if (sb == NULL) {
                printk("make fs on device %x failed!\n", device->super.devno);
                continue;
            }
        }

        /* 磁盘上已经有文件系统，并且返回了其超级块，进行挂载 */
        if (BOFS_MountFileSystem(sb)) {
            printk("mount fs on device %x failed!\n", device->super.devno);
            continue;
        }

        /* 挂载成功后，需要添加磁盘符 */
        memset(driveName, 0 , BOFS_DRIVE_NAME_LEN);
        if (device->super.devno == BOFS_ROOT_DRIVE_DEV) {
            strcpy(driveName, BOFS_ROOT_DRIVE_NAME);
        } else if (device->super.devno == BOFS_SYS_DRIVE_DEV) {
            strcpy(driveName, BOFS_SYS_DRIVE_NAME);
        } else {
            strcpy(driveName, device->super.name);
        }
        
        BOFS_AddDrive(driveName, sb, device);
    }

    BOFS_ListDrive();

    PART_END();
    return 0;
}
