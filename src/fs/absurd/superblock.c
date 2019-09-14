/*
 * file:		fs/absurd/main.c
 * auther:		Jason Hu
 * time:		2019/9/1
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/slab.h>
#include <book/debug.h>
#include <share/string.h>
#include <book/deviceio.h>
#include <share/string.h>

#include <fs/absurd/superblock.h>

struct SuperBlock *superBlockTable[MAX_SUPER_BLOCK_NR];

/**
 * InitSuperBlock - 初始化升级块表
 */
PUBLIC int InitSuperBlockTable()
{
    int i;
    for (i = 0; i < MAX_SUPER_BLOCK_NR; i++) {
        /* 分配内存空间 */
        superBlockTable[i] = (struct SuperBlock *)kmalloc(SECTOR_SIZE,
            GFP_KERNEL);
        if (superBlockTable[i] == NULL) {
            return -1;
        }
        memset(superBlockTable[i], 0, SECTOR_SIZE);
    }
    return 0;
}

PUBLIC struct SuperBlock *AllocSuperBlock()
{
    int i;
    for (i = 0; i < MAX_SUPER_BLOCK_NR; i++) {
        /* 如果还没有魔数，表示还没有被分配 */
        if (superBlockTable[i]->magic != ABSURD_SUPER_BLOCK_MAGIC) {
            return superBlockTable[i];
        }
    }
    return NULL;
}

PUBLIC void FreeSuperBlock(struct SuperBlock *sb)
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

PUBLIC int LoadMetaIdBitmap(struct SuperBlock *sb)
{
    sb->metaIdBitmap.bits = kmalloc(sb->metaIdBitmapSectors * SECTOR_SIZE,
        GFP_KERNEL);
    if (sb->metaIdBitmap.bits == NULL) {
        printk(PART_ERROR "kmalloc for meta id bitmap failed!\n");
        return -1;
    }

    /* 从磁盘上读取到内存中 */
    if (DeviceRead(sb->meta.deviceID, sb->metaIdBitmapLba,
        sb->metaIdBitmap.bits, sb->metaIdBitmapSectors)) {
        
        printk(PART_ERROR "read meta id bitmap failed!\n");
        return -1;
    }

    return 0;
}

PUBLIC int LoadSectorBitmap(struct SuperBlock *sb)
{
    sb->sectorBitmap.bits = kmalloc(sb->sectorBitmapSectors * SECTOR_SIZE,
        GFP_KERNEL);
    if (sb->sectorBitmap.bits == NULL) {
        printk(PART_ERROR "kmalloc for sector bitmap failed!\n");
        return -1;
    }

    /* 从磁盘上读取到内存中 */
    if (DeviceRead(sb->meta.deviceID, sb->sectorBitmapLba,
        sb->sectorBitmap.bits, sb->sectorBitmapSectors)) {
        
        printk(PART_ERROR "read sector bitmap failed!\n");
        return -1;
    }

    return 0;
}

/**
 * AllocMetaId - 分配元信息ID
 * @sb: 超级块
 * @count: 需要多少个元信息
 */
PUBLIC unsigned int AllocMetaId(struct SuperBlock *sb, int count)
{
    /* 扫描一个位 */
    int idx = BitmapScan(&sb->metaIdBitmap, count);

    if (idx == -1)
        return -1;
    
    /* 置1，表明已经使用 */
    int i;
    for (i = 0; i < count; i++) 
        BitmapSet(&sb->metaIdBitmap, idx + i, 1);

    return idx;
}

/**
 * FreeMetaId - 释放元信息ID
 * @sb: 超级块
 * @id: id数
 * @count: 释放多少个id
 */
PUBLIC void FreeMetaId(struct SuperBlock *sb, unsigned int id, int count)
{
    if (id == -1)
        return;
    
    /* 置0，表明已经释放 */
    int i;
    for (i = 0; i < count; i++) 
        BitmapSet(&sb->metaIdBitmap, id + i, 0);
    
}

/**
 * AllocSector - 分配扇区
 * @sb: 超级块
 * @count: 需要多少个扇区
 */
PUBLIC unsigned int AllocSector(struct SuperBlock *sb, int count)
{
    /* 扫描一个位 */
    int idx = BitmapScan(&sb->sectorBitmap, count);

    if (idx == -1)
        return -1;
    
    /* 置1，表明已经使用 */
    int i;
    for (i = 0; i < count; i++) 
        BitmapSet(&sb->sectorBitmap, idx + i, 1);

    return idx;
}

/**
 * FreeSector - 释放扇区
 * @sb: 超级块
 * @lba: 扇区地址
 * @count: 释放多少个扇区
 */
PUBLIC void FreeSector(struct SuperBlock *sb, unsigned int lba, int count)
{
    if (lba == -1)
        return;
    
    /* 置0，表明已经释放 */
    int i;
    for (i = 0; i < count; i++) 
        BitmapSet(&sb->sectorBitmap, lba + i, 0);
    
}
