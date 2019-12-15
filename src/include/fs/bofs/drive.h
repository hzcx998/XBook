/*
 * file:		include/fs/bofs/driver.h
 * auther:		Jason Hu
 * time:		2019/11/30
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOFS_DRIVE_H
#define _BOFS_DRIVE_H

#include <book/blk-dev.h>

#include <fs/bofs/super_block.h>

#define BOFS_DRIVE_NAME_LEN     24

/* 磁盘符结构 */
struct BOFS_Drive {
    struct List list;                       /* 所有磁盘符构成一个链表 */
    char name[BOFS_DRIVE_NAME_LEN];         /* 磁盘符的名字 */
    struct BOFS_SuperBlock *sb;             /* 磁盘符对应得超级块 */
    struct BlockDevice *blkdev;             /* 块设备 */
};

/* 把哪个设备映射成根磁盘符 */
#define BOFS_ROOT_DRIVE_DEV       DEV_HDA

/* 把哪个设备映射成系统磁盘符 */
#define BOFS_SYS_DRIVE_DEV        DEV_RDA

/* 根磁盘符的名字 */
#define BOFS_ROOT_DRIVE_NAME        "root"
/* 系统磁盘符的名字 */
#define BOFS_SYS_DRIVE_NAME         "sys"

PUBLIC int BOFS_AddDrive(char *name,
    struct BOFS_SuperBlock *sb,
    struct BlockDevice *blkdev);
PUBLIC int BOFS_DelDrive(char *name);

PUBLIC struct BOFS_Drive *BOFS_GetDriveByName(char *name);

PUBLIC void BOFS_ListDrive();
PUBLIC void BOFS_DumpDrive(struct BOFS_Drive *drive);

PUBLIC int BOFS_ChangeCWD(const char *pathname, struct BOFS_Drive *drive);

#endif

