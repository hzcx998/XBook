/*
 * file:		fs/bofs/dir_entry.c
 * auther:		Jason Hu
 * time:		2019/9/6
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/string.h>
#include <drivers/ide.h>
#include <fs/bofs/bitmap.h>
#include <book/blk-buffer.h>

/**
 * BOFS_LoadBitmap - 加载位图
 * @sb: 超级块
 * 
 * 把扇区位图和节点位图加载从磁盘到内存
 * @return: 成功0，失败-1
 */
PUBLIC int BOFS_LoadBitmap(struct BOFS_SuperBlock *sb)
{
    /* 位图长度是格式化的时候就已经写入了的，这里不需要设定值，直接用就是了 */
    
    sb->sectorBitmap.bits = (unsigned char *)kmalloc(sb->sectorBitmap.btmpBytesLen,
        GFP_KERNEL);
    if (sb->sectorBitmap.bits == NULL) {
        return -1;
    }
    //printk("sector bitmap %x len %x\n", sb->sectorBitmap.bits, sb->sectorBitmap.btmpBytesLen);
	
    BitmapInit(&sb->sectorBitmap);

	int i;
	for (i = 0; i < sb->sectorBitmapSectors; i++) {
		if (BlockRead(sb->devno, sb->sectorBitmapLba + i, sb->sectorBitmap.bits + i * sb->blockSize)) {
			printk(PART_ERROR "device read failed!\n");
			return -1;
		}
	}

	sb->inodeBitmap.bits = (uint8 *)kmalloc(sb->inodeBitmap.btmpBytesLen,
        GFP_KERNEL);
        
    if (sb->sectorBitmap.bits == NULL) {
        return -1;
    }    
    //printk("inode bitmap %x len %x\n", sb->inodeBitmap.bits, sb->inodeBitmap.btmpBytesLen);
	
	BitmapInit(&sb->inodeBitmap);
    
	for (i = 0; i < sb->sectorBitmapSectors; i++) {
		if (BlockRead(sb->devno, sb->inodeBitmapLba + i, sb->inodeBitmap.bits + i * sb->blockSize)) {
			
			printk(PART_ERROR "device read failed!\n");
			return -1;
		}
	}
    return 0;
}

/**
 * BOFS_AllocBitmap - 从位图中分配位
 * @sb: 超级块
 * @bmType: 位图类型
 * @counts: 位的数量
 * 
 * @return 成功返回索引下标，失败返回-1
 */
PUBLIC int BOFS_AllocBitmap(struct BOFS_SuperBlock *sb,
    enum BOFS_BM_TYPE bmType, uint32 counts)
{
	int idx, i;
	if(bmType == BOFS_BMT_SECTOR){
		idx = BitmapScan(&sb->sectorBitmap, counts);
		//printk("counts:%d idx:%d\n", counts, idx);
		//printk("<<<alloc sector idx:%d\n", idx);
		if(idx == -1){
			printk("alloc sector bitmap failed!\n");
			return -1;
		}
		for(i = 0; i < counts; i++){
			BitmapSet(&sb->sectorBitmap, idx + i, 1);
		}
		return idx;
	}else if(bmType == BOFS_BMT_INODE){
		
		idx = BitmapScan(&sb->inodeBitmap, counts);
		//printk("counts:%d idx:%d\n", counts, idx);
		
		if(idx == -1){
			printk("alloc inode bitmap failed!\n");
			return -1;
		}
		for(i = 0; i < counts; i++){
			BitmapSet(&sb->inodeBitmap, idx + i, 1);
		}
		return idx;
	}
	return -1;
}

/**
 * BOFS_FreeBitmap - 从位图中释放位
 * @sb: 超级块
 * @bmType: 位图类型
 * @idx: 位的索引
 * 
 * @return 成功返回索引下标，失败返回-1
 */
PUBLIC int BOFS_FreeBitmap(struct BOFS_SuperBlock *sb,
    enum BOFS_BM_TYPE bmType, uint32 idx)
{
	if(bmType == BOFS_BMT_SECTOR){
		//printk(">>>free sector idx:%d\n", idx);
		if(idx == -1){
			printk(PART_ERROR "free sector bitmap failed!\n");
			return -1;
		}
		BitmapSet(&sb->sectorBitmap, idx, 0);
		return idx;
	}else if(bmType == BOFS_BMT_INODE){
		if(idx == -1){
			printk(PART_ERROR "free inode bitmap failed!\n");
			return -1;
		}
		BitmapSet(&sb->inodeBitmap, idx, 0);
		return idx;
	}
	return -1;
}

/**
 * BOFS_SyncBitmap - 把索引对应的位同步到磁盘
 * @sb: 超级块
 * @bmType: 位图类型
 * @idx: 位的索引
 * 
 * @return 成功返回索引下标，失败返回-1
 */  
PUBLIC int BOFS_SyncBitmap(struct BOFS_SuperBlock *sb,
    enum BOFS_BM_TYPE bmType, uint32 idx)
{
	uint32 offsetSector = idx / (8*SECTOR_SIZE);
	uint32 offsetSize = offsetSector * SECTOR_SIZE;
	uint32 sectorLba;
	uint8 *bitmapOffset;
	if(bmType == BOFS_BMT_SECTOR){
		//printk("^^^sync sector idx:%d\n", idx);
		sectorLba = sb->sectorBitmapLba + offsetSector;
		bitmapOffset = sb->sectorBitmap.bits + offsetSize;
		
		if (BlockWrite(sb->devno, sectorLba, bitmapOffset, 0)) {
            printk(PART_ERROR "device %d write failed!\n", sb->devno);
            return -1;
        }
		return true;
	}else if(bmType == BOFS_BMT_INODE){
		
		sectorLba = sb->inodeBitmapLba + offsetSector;
		bitmapOffset = sb->inodeBitmap.bits + offsetSize;
		
        if (BlockWrite(sb->devno, sectorLba, bitmapOffset, 0)) {
            printk(PART_ERROR "device %d write failed!\n", sb->devno);
            return -1;
        }
		return true;
	}
	return false;
}