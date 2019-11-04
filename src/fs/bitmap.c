/*
 * file:		fs/bitmap.c
 * auther:		Jason Hu
 * time:		2019/11/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/string.h>
#include <fs/bitmap.h>
#include <book/blk-buffer.h>
#include <fs/super_block.h>
/**
 * LoadSuperBlockBitmap - 加载超级块中的位图
 * @sb: 超级块
 * 
 * 把块位图和节点位图加载从磁盘到内存
 * @return: 成功0，失败-1
 */
PUBLIC int LoadSuperBlockBitmap(struct SuperBlock *sb)
{
	/* 加载块位图到内存 */
	sb->blockBitmap.bits = kmalloc(sb->blockBitmapBlocks *
		sb->blockSize, GFP_KERNEL);
	if (sb->blockBitmap.bits == NULL) {
		printk("kmalloc for block bitmap failed!\n");
		return -1;
	}
	BitmapInit(&sb->blockBitmap);
	
	/*memset(sb->blockBitmap.bits, 0, sb->blockBitmapBlocks *
		sb->blockSize);
	*/
	int i;
	for (i = 0; i < sb->blockBitmapBlocks; i++) {	
		if (!BlockRead(sb->devno, sb->blockBitmapLba + i,
			sb->blockBitmap.bits + i * sb->blockSize))
		{
			printk("block read for block bitmap failed!\n");
			kfree(sb->blockBitmap.bits);
			return -1;
		}
	}

	/* 加载节点位图到内存 */
	sb->nodeBitmap.bits = kmalloc(sb->nodeBitmapBlocks *
		sb->blockSize, GFP_KERNEL);
	if (sb->nodeBitmap.bits == NULL) {
		printk("kmalloc for node bitmap failed!\n");
		kfree(sb->blockBitmap.bits);
		return -1;
	}
	BitmapInit(&sb->nodeBitmap);
	/*
	memset(sb->nodeBitmap.bits, 0, sb->nodeBitmapBlocks *
		sb->blockSize);
	*/
	for (i = 0; i < sb->nodeBitmapBlocks; i++) {	
		if (!BlockRead(sb->devno, sb->nodeBitmapLba + i,
			sb->nodeBitmap.bits + i * sb->blockSize)) 
		{
			printk("block read for block bitmap failed!\n");
			kfree(sb->blockBitmap.bits);
			kfree(sb->nodeBitmap.bits);
			return -1;
		}
	}
	
	sb->iobuf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (sb->iobuf == NULL) {
		printk("kmalloc for block bitmap failed!\n");
		kfree(sb->blockBitmap.bits);
		kfree(sb->nodeBitmap.bits);
		return -1;
	}

	return 0;
}

/**
 * FlatAllocBitmap - 从位图中分配位
 * @sb: 超级块
 * @bmType: 位图类型
 * @counts: 位的数量
 * 
 * @return 成功返回索引下标，失败返回-1
 */
PUBLIC int FlatAllocBitmap(struct SuperBlock *sb,
    enum FLAT_BM_TYPE bmType, size_t counts)
{
	int idx, i;
	if(bmType == FLAT_BMT_BLOCK){
		idx = BitmapScan(&sb->blockBitmap, counts);
		//printk("counts:%d idx:%d\n", counts, idx);
		//printk("<<<alloc sector idx:%d\n", idx);
		if(idx == -1){
			printk("alloc block bitmap failed!\n");
			return -1;
		}
		for(i = 0; i < counts; i++){
			BitmapSet(&sb->blockBitmap, idx + i, 1);
		}
		return idx;
	}else if(bmType == FLAT_BMT_NODE){
		
		idx = BitmapScan(&sb->nodeBitmap, counts);
		//printk("counts:%d idx:%d\n", counts, idx);
		
		if(idx == -1){
			printk("alloc node bitmap failed!\n");
			return -1;
		}
		for(i = 0; i < counts; i++){
			BitmapSet(&sb->nodeBitmap, idx + i, 1);
		}
		return idx;
	}
	return -1;
}

/**
 * FlatFreeBitmap - 从位图中释放位
 * @sb: 超级块
 * @bmType: 位图类型
 * @idx: 位的索引
 * 
 * @return 成功返回索引下标，失败返回-1
 */
PUBLIC int FlatFreeBitmap(struct SuperBlock *sb,
    enum FLAT_BM_TYPE bmType, uint32 idx)
{
	if(bmType == FLAT_BMT_BLOCK){
		//printk(">>>free sector idx:%d\n", idx);
		if(idx == -1){
			printk(PART_ERROR "free block bitmap failed!\n");
			return -1;
		}
		BitmapSet(&sb->blockBitmap, idx, 0);
		return idx;
	}else if(bmType == FLAT_BMT_NODE){
		if(idx == -1){
			printk(PART_ERROR "free node bitmap failed!\n");
			return -1;
		}
		BitmapSet(&sb->blockBitmap, idx, 0);
		return idx;
	}
	return -1;
}

/**
 * FlatSyncBitmap - 把索引对应的位同步到磁盘
 * @sb: 超级块
 * @bmType: 位图类型
 * @idx: 位的索引
 * 
 * @return 成功返回1，失败返回0
 */
PUBLIC int FlatSyncBitmap(struct SuperBlock *sb,
    enum FLAT_BM_TYPE bmType, unsigned int idx)
{
	/* 在位图磁盘中的哪个块中 */
	off_t Blockoffset = idx / (8 * sb->blockSize);

	unsigned int lba;
	unsigned char *bitsOffset;
	if(bmType == FLAT_BMT_BLOCK){
		//printk("^^^sync sector idx:%d\n", idx);
		lba = sb->blockBitmapLba + Blockoffset;
		bitsOffset = sb->blockBitmap.bits + Blockoffset * sb->blockSize;
	}else if(bmType == FLAT_BMT_NODE){
		lba = sb->nodeBitmapLba + Blockoffset;
		bitsOffset = sb->nodeBitmap.bits + Blockoffset * sb->blockSize;
	}

	/* 只对一个块进行操作 */
	if (!BlockWrite(sb->devno, lba, bitsOffset, 0)) {
		printk(PART_ERROR "device %d write failed!\n", sb->devno);
		return -1;
	}
	return 1;
}
