/*
 * file:		kernel/fs/bofs/device.c
 * auther:		Jason Hu
 * time:		2019/11/30
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <book/device.h>

#include <lib/string.h>
#include <lib/string.h>
#include <lib/vsprintf.h>

#include <fs/bofs/inode.h>
#include <fs/bofs/dir_entry.h>
#include <fs/bofs/super_block.h>
#include <fs/bofs/bitmap.h>
#include <fs/bofs/drive.h>

/* 创建一个磁盘符链表 */
LIST_HEAD(driveListHead);


/**
 * BOFS_GetDriveByName - 通过名字搜索磁盘符
 * @name: 磁盘符名字
 * 
 * 成功返回磁盘符，失败返回NULL
 */
PUBLIC struct BOFS_Drive *BOFS_GetDriveByName(char *name)
{
    struct BOFS_Drive *drive;
    ListForEachOwner(drive, &driveListHead, list) {
        if (!strcmp(drive->name, name))
            return drive;
    }
    return NULL;
}

/**
 * BOFS_AddDrive - 添加一个磁盘符
 * @name: 磁盘符名字
 * @sb: 超级块
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int BOFS_AddDrive(char *name,
    struct BOFS_SuperBlock *sb,
    struct BlockDevice *blkdev)
{
    if (BOFS_GetDriveByName(name) != NULL) {
        printk("drive %s has exited!\n", name);
        return -1;
    }

    struct BOFS_Drive *drive = kmalloc(sizeof(struct BOFS_Drive), GFP_KERNEL);
    if (drive == NULL) {
        return -1;
    }
    memset(drive->name, 0, BOFS_DRIVE_NAME_LEN);
    strcpy(drive->name, name);

    ListAddTail(&drive->list, &driveListHead);

    drive->sb = sb;
    drive->blkdev = blkdev;

    return 0;
}

/**
 * BOFS_DelDrive - 删除一个磁盘符
 * @name: 磁盘符名字
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int BOFS_DelDrive(char *name)
{
    struct BOFS_Drive *drive = BOFS_GetDriveByName(name);
    if (drive == NULL)
        return -1;

    ListDel(&drive->list);

    kfree(drive);
    return 0;
}



PUBLIC void BOFS_DumpDrive(struct BOFS_Drive *drive)
{
    printk(PART_TIP "----BOFS Drive----\n");

    printk(PART_TIP "name:%s super block:%x block device:%x\n",
        drive->name, drive->sb, drive->blkdev);

}

PUBLIC void BOFS_ListDrive()
{
    printk("list drives\n");
    struct BOFS_Drive *drive;
    ListForEachOwner(drive, &driveListHead, list) {
       printk("drive: %s\n", drive->name); 
    }
}
