/*
 * file:		fs/bofs/super_block.c
 * auther:		Jason Hu
 * time:		2019/9/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/slab.h>
#include <book/debug.h>
#include <share/string.h>
#include <book/deviceio.h>
#include <share/string.h>
#include <driver/ide.h>
#include <fs/bofs/super_block.h>

/*we have 26 super blocks in memory*/
#define MAX_SUPER_BLOCK_NR 26

/*super block talbe*/
struct BOFS_SuperBlock *superBlockTable[MAX_SUPER_BLOCK_NR];

/* 主超级块 */
struct BOFS_SuperBlock *masterSuperBlock, *currentSuperBlock;

/**
 * InitSuperBlock - 初始化升级块表
 */
PUBLIC int BOFS_InitSuperBlockTable()
{
    int i;
    for (i = 0; i < MAX_SUPER_BLOCK_NR; i++) {
        /* 分配内存空间 */
        superBlockTable[i] = (struct BOFS_SuperBlock *)kmalloc(SECTOR_SIZE,
            GFP_KERNEL);
        if (superBlockTable[i] == NULL) {
            return -1;
        }
        memset(superBlockTable[i], 0, SECTOR_SIZE);
    }
    return 0;
}

PUBLIC struct BOFS_SuperBlock *BOFS_AllocSuperBlock()
{
    int i;
    for (i = 0; i < MAX_SUPER_BLOCK_NR; i++) {
        /* 如果还没有魔数，表示还没有被分配 */
        if (superBlockTable[i]->magic != BOFS_SUPER_BLOCK_MAGIC) {
            return superBlockTable[i];
        }
    }
    return NULL;
}

PUBLIC void BOFS_FreeSuperBlock(struct BOFS_SuperBlock *sb)
{
    int i;
    for (i = 0; i < MAX_SUPER_BLOCK_NR; i++) {
        /* 必须要在表中 */
        if (superBlockTable[i] == sb) {
            /* 把magic置0表示没有使用 */
            sb->magic = 0;
            break;
        }
    }
}

PUBLIC void BOFS_DumpSuperBlock(struct BOFS_SuperBlock *sb)
{
    printk(PART_TIP "----Super Block----\n");
    printk(PART_TIP "device id:%d sectors:%d sector bitmap lba:%d sector bitmap sectors:%d\n", 
        sb->deviceID, sb->totalSectors, sb->sectorBitmapLba, 
        sb->sectorBitmapSectors);
    printk(PART_TIP "inode bitmap lba:%d inode bitmap sectors:%d \
inode table lba:%d inode table sectors:%d data start lba:%d\n", 
        sb->inodeBitmapLba, sb->inodeBitmapSectors, 
        sb->inodeTableLba, sb->inodeTableSectors, 
        sb->dataStartLba);
}

PUBLIC int BOFS_SuperBlockLoadBuffer(struct BOFS_SuperBlock *sb)
{
    int i;
    for (i = 0; i < BOFS_SUPER_BLOCK_IOBUF_NR; i++) {
        sb->databuf[i] = kmalloc(SECTOR_SIZE, GFP_KERNEL);
        if (sb->databuf[i] == NULL) {
            return -1;
        } 
        memset(sb->databuf[i], 0, SECTOR_SIZE);
    }

    sb->iobuf = kmalloc(SECTOR_SIZE, GFP_KERNEL);
    if (sb->iobuf == NULL) {
        return -1;
    } 
    memset(sb->iobuf, 0, SECTOR_SIZE);
    return 0;
}
