/*
 * file:		fs/bofs/inode.c
 * auther:		Jason Hu
 * time:		2019/9/6
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/string.h>
#include <fs/bofs/inode.h>
#include <drivers/clock.h>
#include <fs/bofs/bitmap.h>
#include <share/math.h>
#include <book/blk-buffer.h>

/**
 * BOFS_CloseInode - 关闭一个节点
 * @inode: 要关闭的节点
 * 
 */
PUBLIC void BOFS_CloseInode(struct BOFS_Inode *inode)
{
	if(inode != NULL){
		kfree(inode);
	}
}

/**
 * BOFS_CreateInode - 创建一个节点
 * @inode: 节点地址
 * @id: 节点id
 * @mode: 节点的模式
 * @flags: 节点的标志
 * @devno: 设备号（devno）
 * 
 * 在节点中填写基础数据
 */
PUBLIC void BOFS_CreateInode(struct BOFS_Inode *inode,
    unsigned int id,
    unsigned int mode,
    unsigned int flags,
    dev_t devno)
{
	memset(inode, 0, sizeof(struct BOFS_Inode));
	inode->id = id;
	inode->mode = mode;
	inode->links = 0;
	inode->size = 0;
	
	inode->crttime = SystemDateToData();	/*create time*/
	inode->acstime = inode->mdftime = inode->crttime;	/*modify time*/
	
	inode->flags = flags;	/* 访问标志 */
	
    inode->devno = devno;	/* 设备ID */
	
	int i;
	for(i = 0; i < BOFS_BLOCK_NR; i++){
		inode->blocks[i] = 0;
	}
}

/**
 * BOFS_DumpInode - 调试输出节点
 * @inode: 要输出的节点
 */
PUBLIC void BOFS_DumpInode(struct BOFS_Inode *inode)
{
    printk(PART_TIP "---- Inode ----\n");
    printk(PART_TIP "id:%d mode:%x links:%d size:%x flags:%x block start:%d\n",
        inode->id, inode->mode, inode->links, inode->size,
        inode->flags, inode->blocks[0]);

    printk(PART_TIP "Date: %d/%d/%d",
        DATA16_TO_DATE_YEA(inode->crttime >> 16),
        DATA16_TO_DATE_MON(inode->crttime >> 16),
        DATA16_TO_DATE_DAY(inode->crttime >> 16));
    printk(" %d:%d:%d\n",
        DATA16_TO_TIME_HOU(inode->crttime),
        DATA16_TO_TIME_MIN(inode->crttime),
        DATA16_TO_TIME_SEC(inode->crttime));
}

/**
 * BOFS_SyncInode - 把节点同步到磁盘
 * @inode: 要同步的节点
 * @sb: 节点所在的超级块
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int BOFS_SyncInode(struct BOFS_Inode *inode, struct BOFS_SuperBlock *sb)
{
	uint32 sectorOffset = inode->id / sb->inodeNrInSector;
	uint32 lba = sb->inodeTableLba + sectorOffset;
	uint32 bufOffset = inode->id % sb->inodeNrInSector;
	
	//printk("BOFS sync inode: sec off:%d lba:%d buf off:%d\n", sectorOffset, lba, bufOffset);
	
    /* 分配IO缓冲区 */
    buf8_t iobuf = kmalloc(sb->blockSize, GFP_KERNEL);
    if (iobuf == NULL) {
        return -1;
    }
    
	memset(iobuf, 0, sb->blockSize);
    if (BlockRead(sb->devno, lba, iobuf)) {
        kfree(iobuf);
        return -1;
    }

	struct BOFS_Inode *inode2 = (struct BOFS_Inode *)iobuf;
	*(inode2 + bufOffset) = *inode;
	
    //printk("ready to write!\n");
    if (BlockWrite(sb->devno, lba, iobuf, 0)) {
        kfree(iobuf);
        return -1;
    }

    kfree(iobuf);
    
    //printk("sync success!\n");
    return 0;
}

/**
 * BOFS_EmptyInode - 清空节点的信息
 * @inode: 节点
 * @sb: 节点所在的超级块
 * 
 * 从磁盘上吧节点的所有信息清空
 * 成功返回0，失败返回-1
 */
PUBLIC int BOFS_EmptyInode(struct BOFS_Inode *inode, struct BOFS_SuperBlock *sb)
{
	uint32 sectorOffset = inode->id/sb->inodeNrInSector;
	uint32 lba = sb->inodeTableLba + sectorOffset;
	uint32 bufOffset = inode->id % sb->inodeNrInSector;
	
	//printk("bofs sync: sec off:%d lba:%d buf off:%d\n", sectorOffset, lba, bufOffset);
	/* 分配IO缓冲区 */
    buf8_t iobuf = kmalloc(sb->blockSize, GFP_KERNEL);
    if (iobuf == NULL) {
        return -1;
    }

	memset(iobuf, 0, sb->blockSize);
	
	if (BlockRead(sb->devno, lba, iobuf)) {
        kfree(iobuf);
		return -1;
	}

	struct BOFS_Inode *inodeBuf = (struct BOFS_Inode *)iobuf;

	memset(&inodeBuf[bufOffset], 0, sizeof(struct BOFS_Inode));
	
	if (BlockWrite(sb->devno, lba, iobuf, 0)) {
        kfree(iobuf);
		return -1;
	}

    kfree(iobuf);
	return 0;
}

/**
 * BOFS_LoadInodeByID - 通过inode id 加载节点信息
 * @inode: 储存节点信息
 * @id: 节点id
 * @sb: 所在的超级块
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int BOFS_LoadInodeByID(struct BOFS_Inode *inode, 
    unsigned int id, 
    struct BOFS_SuperBlock *sb)
{

	uint32 sectorOffset = id / sb->inodeNrInSector;
	uint32 lba = sb->inodeTableLba + sectorOffset;
	uint32 bufOffset = id % sb->inodeNrInSector;
	
	//printk("BOFS load inode sec off:%d lba:%d buf off:%d\n", sectorOffset, lba, bufOffset);
	//printk("BOFS inode nr in sector %d\n", sb->inodeNrInSector);
	
    /* 分配IO缓冲区 */
    buf8_t iobuf = kmalloc(sb->blockSize, GFP_KERNEL);
    if (iobuf == NULL) {
        return -1;
    }

	memset(iobuf, 0, sb->blockSize);
	if (BlockRead(sb->devno, lba, iobuf)) {
        kfree(iobuf);
        return -1;
    }

	struct BOFS_Inode *inode2 = (struct BOFS_Inode *)iobuf;
	*inode = inode2[bufOffset];
	//BOFS_DumpInode(inode);

    kfree(iobuf);
	return 0;
}

/**
 * BOFS_CopyInodeData - 复制节点的数据
 * @dst: 目的节点
 * @src: 源节点
 * @sb: 超级块
 * 
 * 复制节点对应的数据，而不是节点
 * 成功返回0，失败返回-1
 */
PUBLIC int BOFS_CopyInodeData(struct BOFS_Inode *dst,
	struct BOFS_Inode *src,
	struct BOFS_SuperBlock *sb)
{
	/*if size is 0, don't copy data*/
	if(src->size == 0){
		return -1;
	}
	/*inode data blocks*/
	uint32 blocks = DIV_ROUND_UP(src->size, sb->blockSize);
	uint32 blockID = 0;
	
	uint32 srcLba, dstLba;
	
    /* 分配IO缓冲区 */
	buf8_t iobuf = kmalloc(sb->blockSize, GFP_KERNEL);
	if (iobuf == NULL) {
		return -1;
	}

	while(blockID < blocks){
		/*get a lba and read data*/
		BOFS_GetInodeData(src, blockID, &srcLba, sb);
		memset(iobuf, 0, sb->blockSize);
		if (BlockRead(sb->devno, srcLba, iobuf)) {
			return -1;
		}
		
		/*get a lba and write data*/
		BOFS_GetInodeData(dst, blockID, &dstLba, sb);
		if (BlockWrite(sb->devno, dstLba, iobuf, 0)) {
			return -1;
		}
		blockID++;
	}
	kfree(iobuf);
	return 0;
}

/**
 * BOFS_CopyInode - 复制节点
 * @dst: 目的节点
 * @src: 源节点
 * 
 * 复制节点，而不是节点的数据
 * 成功返回0，失败返回-1    
 */
PUBLIC void BOFS_CopyInode(struct BOFS_Inode *dst, struct BOFS_Inode *src)
{
	memset(dst, 0, sizeof(struct BOFS_Inode));
	
	dst->id = src->id;
	
	dst->mode = src->mode;
	dst->links = src->links;
	
	dst->size = src->size;
	
	dst->crttime = src->crttime;
	dst->mdftime = src->mdftime;
	dst->acstime = src->acstime;
	
	dst->flags = src->flags;

}

PRIVATE int AllocOneSector(struct BOFS_SuperBlock *sb, unsigned int *data)
{
	int idx = BOFS_AllocBitmap(sb, BOFS_BMT_SECTOR, 1);
	if(idx == -1){
		printk("alloc block bitmap failed!\n");
		return -1;
	}
	BOFS_SyncBitmap(sb, BOFS_BMT_SECTOR, idx);
	*data = BOFS_IDX_TO_LBA(sb, idx); 
	//printk("d%d ",*data);
	
	/* 清空块里面的数据 */
	buf8_t buffer = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buffer == NULL) {
		printk(PART_ERROR "kmalloc for buf failed!\n");
		return -1;
	}
	
	memset(buffer, 0, sb->blockSize);
	if (BlockWrite(sb->devno, *data, buffer, 0)) {
		printk(PART_ERROR "device %d write failed!\n", sb->devno);
		return -1;
	}
	
	kfree(buffer);
	
	return 0;
}

PRIVATE int FreeOneSector(struct BOFS_SuperBlock *sb, unsigned int lba)
{
	//printk("block %d start %d\n", block, sb->dataStartLba);

	/* 释放块位图 */
	int idx = BOFS_LBA_TO_IDX(sb, lba);
	//printk("<< %d\n", idx);
	
	BOFS_FreeBitmap(sb, BOFS_BMT_SECTOR ,idx);
	//printk("<<\n");
	
	BOFS_SyncBitmap(sb, BOFS_BMT_SECTOR, idx);
	//printk("<<\n");
	
	return 0;
}

/* 
分级边界，0是直接块，1~3是间接块
根据不同的块的大小，需要进行调整！
 */
#define BLOCK_LV0	12
#define BLOCK_LV1	(BLOCK_LV0 + 256)
#define BLOCK_LV2	(BLOCK_LV1 + 256*256)
#define BLOCK_LV3	(BLOCK_LV2 + 256*256*256)

/* 1级间接块在NodeFile->blocs中的索引 */
#define INDERCT_BLOCKS_LV1_INDEX	12
/* 2级间接块在NodeFile->blocs中的索引 */
#define INDERCT_BLOCKS_LV2_INDEX	13
/* 3级间接块在NodeFile->blocs中的索引 */
#define INDERCT_BLOCKS_LV3_INDEX	14

/**
 * GetBlockInInderect1 - 在1级间接块中获取
 * @node: 节点文件
 * @base: 间接块索引的基数
 * @block: 数据要储存的地方
 * @sb: 超级块
 * 
 * 从1级间接块中获取块地址，成功返回0，失败返回-1
 */
PRIVATE int GetBlockInInderect1(struct BOFS_Inode *inode,
	unsigned int base,
	unsigned int *block,
	struct BOFS_SuperBlock *sb)
{
	/* 默认是失败 */
	int ret = -1;
    
	unsigned int *derect, *inderect1;
	/* 1级块缓冲区 */
	buf32_t buffer1 = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buffer1 == NULL) {
		printk(PART_ERROR "kmalloc for buffer1 failed!\n");
		return -1;
	}
	
	/* 1级间接块是指向节点文件块中的 */
	inderect1 = &inode->blocks[INDERCT_BLOCKS_LV1_INDEX];
	/* 如果间接块没有数据 */
	if (*inderect1 == 0) {
		/* 分配一个数据块 */
		if (AllocOneSector(sb, inderect1)) {
			printk(PART_ERROR "alloc one block failed!\n");
			goto ToEnd;
		}
		//printk("lv 1 alloc one block %d for inderect1.\n", *inderect1);
		/* 修改数据后同步节点文件 */
		BOFS_SyncInode(inode, sb);
	}
    
	/* 读取1级间接块 */
	memset(buffer1, 0, sb->blockSize);

	if (BlockRead(sb->devno, *inderect1, buffer1)) {
		printk(PART_ERROR "device %d read failed!\n", sb->devno);
		goto ToEnd;
	}

	/* 直接块 */
	derect = &buffer1[base];
	/* 如果直接块没有数据 */
	if (*derect == 0) {
		/* 分配一个数据块 */
		if (AllocOneSector(sb, derect)) {
			printk("lv 1 alloc one block failed!\n");
			goto ToEnd;
		}
		//printk("lv 1 alloc one block %d for derect.\n", *derect);
		//printk("%d ",*derect);
	
		/* 直接块修改之后，需要把直接块的值保存，也就是说，需要把
		直接块对应的数据写回磁盘，写回到1级间接块的数据中 */
		if (BlockWrite(sb->devno, *inderect1, buffer1, 0)) {
			printk(PART_ERROR "device %d write failed!\n", sb->devno);
			goto ToEnd;
		} 
	}
	/* 成功 */
	ret = 0;

	*block = *derect;
	#ifdef DEBUG_NODE_DATA
	printk("base %d off %d inderect %d derect %d\n",
		base, *inderect1, *derect);
	#endif
ToEnd:
	kfree(buffer1);
	return ret;
}

/**
 * GetBlockInInderect2 - 在2级间接块中获取
 * @node: 节点文件
 * @base: 间接块索引的基数
 * @block: 数据要储存的地方
 * @sb: 超级块
 * 
 * 从2级间接块中获取块地址，成功返回0，失败返回-1
 */
PRIVATE int GetBlockInInderect2(struct BOFS_Inode *inode,
	unsigned int base,
	unsigned int *block,
	struct BOFS_SuperBlock *sb)
{
	/* 默认是失败 */
	int ret = -1;
	/* 
	derect: 指向直接块的数据指针
	inderect1: 指向1级间接块的数据指针
	inderect2: 指向2级间接块的数据指针
	 */
	unsigned int *derect, *inderect1, *inderect2;

	/* 获取1级间接块中的索引 */
	unsigned int idxInInderect1 = base / 256;
	
	/* 获取2级间接块中的索引 */
	unsigned int idxInInderect2 = base % 256;
	
	/* 1级块缓冲区 */
	buf32_t buffer1 = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buffer1 == NULL) {
		printk(PART_ERROR "kmalloc for buffer failed!\n");
		return -1;
	}
	/* 2级块缓冲区 */
	buf32_t buffer2 = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buffer2 == NULL) {
		printk(PART_ERROR "kmalloc for buffer failed!\n");
		kfree(buffer1);
		return -1;
	}

	/* 1级间接块是指向节点文件块中的 */
	inderect1 = &inode->blocks[INDERCT_BLOCKS_LV2_INDEX];

	/* 如果间接块没有数据 */
	if (*inderect1 == 0) {
		/* 分配一个数据块 */
		if (AllocOneSector(sb, inderect1)) {
			printk(PART_ERROR "alloc one block failed!\n");
			goto ToEnd;
		}
		//printk("lv 2 alloc one block %d for inderect1.\n", *inderect1);
		/* 修改数据后同步节点文件 */
		BOFS_SyncInode(inode, sb);
	}
	
	memset(buffer1, 0, sb->blockSize);

	/* 读取1级间接块 */
	if (BlockRead(sb->devno, *inderect1, buffer1)) {
		printk(PART_ERROR "device %d read failed!\n", sb->devno);
		goto ToEnd;
	}

	/* 2级间接块是指向1级间接块的数据中的 */
	inderect2 = &buffer1[idxInInderect1];

	/* 如果2级间接块没有数据 */
	if (*inderect2 == 0) {
		/* 分配一个数据块 */
		if (AllocOneSector(sb, inderect2)) {
			printk(PART_ERROR "alloc one block failed!\n");
			goto ToEnd;
		}

		//printk("lv 2 alloc one block %d for inderect2.\n", *inderect2);
		
		/* 2级间接块块修改之后，需要把块的值保存，也就是说，需要把
		2级间接块对应的数据写回磁盘，写回到1级间接块的数据中 */
		if (BlockWrite(sb->devno, *inderect1, buffer1, 0)) {
			printk(PART_ERROR "device %d write failed!\n", sb->devno);
			goto ToEnd;
		}
	}
	
	memset(buffer2, 0, sb->blockSize);

	/* 读取2级间接块的数据 */
	if (BlockRead(sb->devno, *inderect2, buffer2)) {
		printk(PART_ERROR "device %d read failed!\n", sb->devno);
		goto ToEnd;
	}

	/* 获取直接块，根据2级间接块的索引，从2级间接块中获取数据 */
	derect = &buffer2[idxInInderect2];

	/* 如果直接块没有数据 */
	if (*derect == 0) {
		/* 创建一个数据块 */
		if (AllocOneSector(sb, derect)) {
			printk("alloc one block failed!\n");
			goto ToEnd;
		}
		//printk("%d ",*derect);
	
		/* 直接块修改之后，需要把直接块的值保存，也就是说，需要把
		直接块对应的数据写回磁盘，写回到2级间接块的数据中 */
		if (BlockWrite(sb->devno, *inderect2, buffer2, 0)) {
			printk(PART_ERROR "device %d write failed!\n", sb->devno);
			goto ToEnd;
		}

	}
	//printk("%d ",*derect);
	
	/* 成功 */
	ret = 0;

	*block = *derect;
	#ifdef DEBUG_NODE_DATA
	printk("base %d idx1 %d idx2 %d inderect1 %d inderect2 %d derect %d\n",
		base, idxInInderect1, idxInInderect2, *inderect1, *inderect2, *derect);
	#endif
ToEnd:
	kfree(buffer1);
	kfree(buffer2);		
	return ret;
}

/**
 * GetBlockInInderect3 - 在3级间接块中获取
 * @node: 节点文件
 * @base: 间接块索引的基数
 * @block: 数据要储存的地方
 * @sb: 超级块
 * 
 * 从3级间接块中获取块地址，成功返回0，失败返回-1
 */
PRIVATE int GetBlockInInderect3(struct BOFS_Inode *inode,
	unsigned int base,
	unsigned int *block,
	struct BOFS_SuperBlock *sb)
{
	/* 默认是失败 */
	int ret = -1;
	/* 
	derect: 指向直接块的数据指针
	inderect1: 指向1级间接块的数据指针
	inderect2: 指向2级间接块的数据指针
	inderect3: 指向3级间接块的数据指针
	 */
	unsigned int *derect, *inderect1, *inderect2, *inderect3;

	unsigned int idxInInderect1 = 0;
	unsigned int idxInInderect2 = 0;
	unsigned int idxInInderect3 = 0;

	//printk("base %d ", base);

	/* 获取间接块中的索引 */
	if (base < 256) {
		idxInInderect3 = base;
	} else if (base < 256 * 256) {
		idxInInderect3 = base % 256;
		idxInInderect2 = base / 256;
	} else if (base < 256 * 256 * 256) {
		idxInInderect1 = base / (256 * 256);
		base -= idxInInderect1 * (256 * 256);
		idxInInderect3 = base % 256;
		idxInInderect2 = base / 256;
	}  

	/* 1级块缓冲区 */
	buf32_t buffer1 = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buffer1 == NULL) {
		printk(PART_ERROR "kmalloc for buffer failed!\n");
		return -1;
	}
	/* 2级块缓冲区 */
	buf32_t buffer2 = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buffer2 == NULL) {
		printk(PART_ERROR "kmalloc for buffer failed!\n");
		kfree(buffer1);
		return -1;
	}
	/* 3级块缓冲区 */
	buf32_t buffer3 = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buffer2 == NULL) {
		printk(PART_ERROR "kmalloc for buffer failed!\n");
		kfree(buffer1);
		kfree(buffer2);
		return -1;
	}

	/* 1级间接块是指向节点文件块中的 */
	inderect1 = &inode->blocks[INDERCT_BLOCKS_LV3_INDEX];

	/* 如果间接块没有数据 */
	if (*inderect1 == 0) {
		/* 分配一个数据块 */
		if (AllocOneSector(sb, inderect1)) {
			printk(PART_ERROR "alloc one block failed!\n");
			goto ToEnd;
		}
		//printk("lv 3 alloc one block %d for inderect1.\n", *inderect1);
		/* 修改数据后同步节点文件 */
		BOFS_SyncInode(inode, sb);
	}

	memset(buffer1, 0, sb->blockSize);

	/* 读取1级间接块 */
	if (BlockRead(sb->devno, *inderect1, buffer1)) {
		printk(PART_ERROR "device %d read failed!\n", sb->devno);
		goto ToEnd;
	}

	/* 2级间接块是指向1级间接块的数据中的 */
	inderect2 = &buffer1[idxInInderect1];

	/* 如果2级间接块没有数据 */
	if (*inderect2 == 0) {
		/* 分配一个数据块 */
		if (AllocOneSector(sb, inderect2)) {
			printk(PART_ERROR "alloc one block failed!\n");
			goto ToEnd;
		}

		//printk("lv 3 alloc one block %d for inderect2.\n", *inderect2);
		
		/* 2级间接块块修改之后，需要把块的值保存，也就是说，需要把
		2级间接块对应的数据写回磁盘，写回到1级间接块的数据中 */
		if (BlockWrite(sb->devno, *inderect1, buffer1, 0)) {
			printk(PART_ERROR "device %d write failed!\n", sb->devno);
			goto ToEnd;
		}
	}
	
	memset(buffer2, 0, sb->blockSize);

	/* 读取2级间接块的数据 */
	if (BlockRead(sb->devno, *inderect2, buffer2)) {
		printk(PART_ERROR "device %d read failed!\n", sb->devno);
		goto ToEnd;
	}

	/* 3级间接块是指向2级间接块的数据中的 */
	inderect3 = &buffer2[idxInInderect2];

	/* 如果3级间接块没有数据 */
	if (*inderect3 == 0) {
		/* 分配一个数据块 */
		if (AllocOneSector(sb, inderect3)) {
			printk(PART_ERROR "alloc one block failed!\n");
			goto ToEnd;
		}
/*
		printk("lv 3 alloc one block %d for inderect3.\n", *inderect3);
		*/
		/* 3级间接块块修改之后，需要把块的值保存，也就是说，需要把
		3级间接块对应的数据写回磁盘，写回到2级间接块的数据中 */
		if (BlockWrite(sb->devno, *inderect2, buffer2, 0)) {
			printk(PART_ERROR "device %d write failed!\n", sb->devno);
			goto ToEnd;
		}
	}
	
	memset(buffer3, 0, sb->blockSize);

	/* 读取3级间接块的数据 */
	if (BlockRead(sb->devno, *inderect3, buffer3)) {
		printk(PART_ERROR "device %d read failed!\n", sb->devno);
		goto ToEnd;
	}

	/* 获取直接块，根据3级间接块的索引，从3级间接块中获取数据 */
	derect = &buffer3[idxInInderect3];

	/* 如果直接块没有数据 */
	if (*derect == 0) {
		/* 创建一个数据块 */
		if (AllocOneSector(sb, derect)) {
			printk("alloc one block failed!\n");
			goto ToEnd;
		}

		/* 直接块修改之后，需要把直接块的值保存，也就是说，需要把
		直接块对应的数据写回磁盘，写回到3级间接块的数据中 */
		if (BlockWrite(sb->devno, *inderect3, buffer3, 0)) {
			printk(PART_ERROR "device %d write failed!\n", sb->devno);
			goto ToEnd;
		}
	}
	//printk("%d ",*derect);
	
	/* 成功 */
	ret = 0;
	*block = *derect;
	#ifdef DEBUG_NODE_DATA
	
	printk("idx1 %d idx2 %d idx2 %d inderect1 %d inderect2 %d inderect3 %d derect %d\n",
		idxInInderect1, idxInInderect2, idxInInderect3, *inderect1, *inderect2, *inderect3, *derect);
	#endif
ToEnd:	
	kfree(buffer1);
	kfree(buffer2);
	kfree(buffer3);
	return ret;
}

/**
 * BOFS_GetInodeData - 通过数据块索引获取块
 * @node: 节点文件
 * @index: 块索引
 * @block: 存储块的地址
 * @sb: 超级块
 * 
 * 
 * 文件的大小依靠次函数的能力
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int BOFS_GetInodeData(struct BOFS_Inode *inode,
 	uint32 index,
	uint32 *block,
	struct BOFS_SuperBlock *sb)
{
	unsigned int base;
	
	/* 分级 */
	if (index < BLOCK_LV0) {
		/* 直接块，12个 */
		base = index;

		/* 可以直接获取直接块 */
		unsigned int *derect = &inode->blocks[base];
		/* 没有数据块 */
		if (*derect == 0) {
			/* 创建一个数据块 */
			if (AllocOneSector(sb, derect)) {
				printk("alloc one block failed!\n");
				return -1;
			}
			
			/* 修改数据后同步回磁盘 */
			BOFS_SyncInode(inode, sb);
		}
		*block = *derect;
		#ifdef DEBUG_NODE_DATA
	
		printk("level 0, index %d base %d derect %d\n", index, base, *derect);
		#endif
	} else if (index < BLOCK_LV1) {
		/* 1级块,12 + 256个 */
		base = index - BLOCK_LV0;
		#ifdef DEBUG_NODE_DATA
		printk("level 1, index %d ", index);
		#endif
		if (GetBlockInInderect1(inode, base, block, sb)) {
			return -1;
		}
	} else if (index < BLOCK_LV2) {
		/* 2级块,12 + 256 + 256 * 256个 */
		base = index - BLOCK_LV1;

		#ifdef DEBUG_NODE_DATA
		printk("level 2, index %d ", index);
		#endif
		if (GetBlockInInderect2(inode, base, block, sb)) {
			return -1;
		}
	} else if (index < BLOCK_LV3) {
		/* 3级块,12 + 256 + 256 * 256 + + 256 * 256 * 256个 */
		base = index - BLOCK_LV2;
		
		#ifdef DEBUG_NODE_DATA
		printk("level 3, index %d ", index);
		#endif
		if (GetBlockInInderect3(inode, base, block, sb)) {
			return -1;
		}
	} else {
		printk(PART_ERROR "index %d out of boundary!\n");
		return -1;
	}
	return 0;
}


/**
 * IsEmprtyBlock - 判断是否为空块 
 * @block: 块地址
 * @size: 块的大小
 * 
 * 检测一个块是否为空，也就是说全部是0
 * 是就返回1，不是就返回0 
 */
PRIVATE int IsEmprtyBlock(void *block, size_t size)
{
	/* 以4字节为单位进行检测检测 */
	buf32_t buf = (buf32_t )block;
	size /= sizeof(int);	/* 大小除以4 */
	int i;
	for (i = 0; i < size; i++) {
		/* 如果遇到非0，就说明不是空块 */
		if (buf[i] != 0) 
			return 0;
	}
	return 1;
}

/**
 * PutBlockInInderect1 - 在1级间接块中释放
 * @node: 节点文件
 * @base: 间接块索引的基数
 * @sb: 超级块
 * 
 * 从1级间接块中获取块地址，成功返回0，失败返回-1
 */
PRIVATE int PutBlockInInderect1(struct BOFS_Inode *inode,
	unsigned int base,
	struct BOFS_SuperBlock *sb)
{
	/* 默认是失败 */
	int ret = -1;

	unsigned int *derect, *inderect1;
	/* 1级块缓冲区 */
	buf32_t buffer1 = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buffer1 == NULL) {
		printk(PART_ERROR "kmalloc for buffer1 failed!\n");
		return -1;
	}
	//printk("\nhas data %d\n", node->blocks[INDERCT_BLOCKS_LV1_INDEX]);
	
	/* 1级间接块是指向节点文件块中的 */
	inderect1 = &inode->blocks[INDERCT_BLOCKS_LV1_INDEX];
	/* 如果1级间接块有数据 */
	if (*inderect1 != 0) {
		//printk("\nhas data %d\n", node->blocks[INDERCT_BLOCKS_LV1_INDEX]);
	
		/* 读取1级间接块 */
		memset(buffer1, 0, sb->blockSize);
		
		if (BlockRead(sb->devno, *inderect1, buffer1)) {
			printk(PART_ERROR "device %d read failed!\n", sb->devno);
			goto ToEnd;
		}
		//printk(">>>\n");

		/* 直接块,base就是偏移 */
		derect = &buffer1[base];
		//printk(">>>\n");

		/*
		int i;
		for (i = 0; i < 10; i++) {
			if (buffer1[i]) 
				printk("%d ", buffer1[i]);
		}*/

		/* 如果直接块有数据 */
		if (*derect != 0) {
			//printk(">>>\n");
			//printk("lv 1 free one block %d for derect.\n", *derect);
			
			if (FreeOneSector(sb, *derect)) {
				printk("free one block failed!\n");
				goto ToEnd;
			}
			
			//printk("lv 1 free one block %d for derect.\n", *derect);
			
			#ifdef DEBUG_NODE_DATA

			printk("lv 1 free one block %d for derect.\n", *derect);
			
			printk("base %d inderect1 %d derect %d\n",
				base, *inderect1, *derect);
			#endif
			*derect = 0;

			/* 检测间接块是否是空块
			是空块，需要把这个间接块释放掉
			不是空块，就把间接块写回磁盘
			 */
			if (IsEmprtyBlock(buffer1, sb->blockSize)) {
				/* 释放1级间接块 */
				if (FreeOneSector(sb, *inderect1)) {
					printk("free one block failed!\n");
					goto ToEnd;
				}
				#ifdef DEBUG_NODE_DATA
				printk("lv 1 free one block %d for inderect1.\n", *inderect1);
				printk("sync node when free blcok.\n");
				
				#endif
				*inderect1 = 0;
				/* 修改数据后同步到节点中去 */
				BOFS_SyncInode(inode, sb);
				//printk("sync node when free blcok.\n");
						
			} else {	
				/* 直接块修改之后，需要把直接块的值保存，也就是说，需要把
				直接块对应的数据写回磁盘，写回到1级间接块的数据中 */
				if (BlockWrite(sb->devno, *inderect1, buffer1, 0)) {
					printk(PART_ERROR "device %d write failed!\n", sb->devno);
					goto ToEnd;
				}
			}
		}
	}

	/* 成功 */
	ret = 0;
	
ToEnd:
	kfree(buffer1);
	return ret;
}

/**
 * PutBlockInInderect2 - 在2级间接块中释放
 * @node: 节点文件
 * @base: 间接块索引的基数
 * @sb: 超级块
 * 
 * 从2级间接块中获取块地址，成功返回0，失败返回-1
 */
PRIVATE int PutBlockInInderect2(struct BOFS_Inode *inode,
	unsigned int base,
	struct BOFS_SuperBlock *sb)
{
	/* 默认是失败 */
	int ret = -1;
	/* 
	derect: 指向直接块的数据指针
	inderect1: 指向1级间接块的数据指针
	inderect2: 指向2级间接块的数据指针
	 */
	unsigned int *derect, *inderect1, *inderect2;
	
	/* 获取1级间接块中的索引 */
	unsigned int idxInInderect1 = base / 256;
	
	/* 获取2级间接块中的索引 */
	unsigned int idxInInderect2 = base % 256;
	
	/* 1级块缓冲区 */
	buf32_t buffer1 = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buffer1 == NULL) {
		printk(PART_ERROR "kmalloc for buffer1 failed!\n");
		return -1;
	}
	/* 2级块缓冲区 */
	buf32_t buffer2 = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buffer2 == NULL) {
		printk(PART_ERROR "kmalloc for buffer1 failed!\n");
		return -1;
	}
	/* 1级间接块是指向节点文件块中的 */
	inderect1 = &inode->blocks[INDERCT_BLOCKS_LV2_INDEX];
	//printk("v%d ",*inderect1);
	
	
	/* 如果1级间接块有数据 */
	if (*inderect1 != 0) {
		/* 读取1级间接块 */
		memset(buffer1, 0, sb->blockSize);
		
		if (BlockRead(sb->devno, *inderect1, buffer1)) {
			printk(PART_ERROR "device %d read failed!\n", sb->devno);
			goto ToEnd;
		}
			
		/* 2级间接块是指向1级间接块的数据中的 */
		inderect2 = &buffer1[idxInInderect1];
		
		/* 如果2级间接块有数据 */
		if (*inderect2 != 0) {
			/* 读取2级间接块 */
			memset(buffer2, 0, sb->blockSize);
			
			if (BlockRead(sb->devno, *inderect2, buffer2)) {
				printk(PART_ERROR "device %d read failed!\n", sb->devno);
				goto ToEnd;
			}
			//printk("v%d ",*inderect2);
		
			/* 直接块是指向2级间接块的数据中的 */
			derect = &buffer2[idxInInderect2];
			
			/* 直接块有数据 */
			if (*derect != 0) {
				//printk("lv 2 free one block %d for derect.\n", *derect);
				//printk("%d ",*derect);	
				
				if (FreeOneSector(sb, *derect)) {
					printk("free one block failed!\n");
					goto ToEnd;
				}
				#ifdef DEBUG_NODE_DATA
				printk("lv 2 free one block %d for derect.\n", *derect);
				printk("base %d idx1 %d idx2 %d inderect1 %d inderect2 %d derect %d\n",
					base, idxInInderect1, idxInInderect2, *inderect1, *inderect2, *derect);
				#endif
				//printk("v%d ",buffer2[idxInInderect2]);
				
				*derect = 0;
				return 0;
				/* 检测2级间接块是否是空块
				是空块，需要把这个间接块释放掉
				不是空块，就把间接块写回磁盘
				*/
				if (IsEmprtyBlock(buffer2, sb->blockSize)) {
					/* 释放2级间接块 */
					if (FreeOneSector(sb, *inderect2)) {
						printk("free one block failed!\n");
						goto ToEnd;
					}
					
					#ifdef DEBUG_NODE_DATA
					printk("lv 2 free one block %d for inderect2.\n", *inderect2);
					#endif
					*inderect2 = 0;
					
					/* 检测1级间接块是否是空块
					是空块，需要把这个间接块释放掉
					不是空块，就把间接块写回磁盘
					*/
					if (IsEmprtyBlock(buffer1, sb->blockSize)) {
						/* 释放1级间接块 */
						if (FreeOneSector(sb, *inderect1)) {
							printk("free one block failed!\n");
							goto ToEnd;
						}
						#ifdef DEBUG_NODE_DATA
						printk("lv 2 free one block %d for inderect1.\n", *inderect1);
						printk("sync node when free blcok.\n");
						#endif
						*inderect1 = 0;
						//printk("sync node when free blcok.\n");
						/* 修改数据后同步到节点中去 */
						BOFS_SyncInode(inode, sb);
						
					} else {	
						/* 2级块修改之后，需要把2级块的值保存，也就是说，需要把
						2级块对应的数据写回磁盘，写回到1级间接块的数据中 */
						if (BlockWrite(sb->devno, *inderect1, buffer1, 0)) {
							printk(PART_ERROR "device %d write failed!\n", sb->devno);
							goto ToEnd;
						}
					}
				} else {	
					/* 直接块修改之后，需要把直接块的值保存，也就是说，需要把
					直接块对应的数据写回磁盘，写回到2级间接块的数据中 */
					if (BlockWrite(sb->devno, *inderect2, buffer2, 0)) {
						printk(PART_ERROR "device %d write failed!\n", sb->devno);
						goto ToEnd;
					}
				}
			}
			
		}		
	}
	/* 成功 */
	ret = 0;
	
ToEnd:
	kfree(buffer1);
	kfree(buffer2);
	return ret;
}


/**
 * PutBlockInInderect3 - 在3级间接块中释放
 * @node: 节点文件
 * @base: 间接块索引的基数
 * @sb: 超级块
 * 
 * 从3级间接块中获取块地址，成功返回0，失败返回-1
 */
PRIVATE int PutBlockInInderect3(struct BOFS_Inode *inode,
	unsigned int base,
	struct BOFS_SuperBlock *sb)
{
	/* 默认是失败 */
	int ret = -1;
	/* 
	derect: 指向直接块的数据指针
	inderect1: 指向1级间接块的数据指针
	inderect2: 指向2级间接块的数据指针
	inderect3: 指向3级间接块的数据指针
	
	 */
	unsigned int *derect, *inderect1, *inderect2, *inderect3;
	
	unsigned int idxInInderect1 = 0;
	unsigned int idxInInderect2 = 0;
	unsigned int idxInInderect3 = 0;

	//printk("base %d ", base);

	/* 获取间接块中的索引 */
	if (base < 256) {
		idxInInderect3 = base;
	} else if (base < 256 * 256) {
		idxInInderect3 = base % 256;
		idxInInderect2 = base / 256;
	} else if (base < 256 * 256 * 256) {
		idxInInderect1 = base / (256 * 256);
		base -= idxInInderect1 * (256 * 256);
		idxInInderect3 = base % 256;
		idxInInderect2 = base / 256;
	}  

	/* 1级块缓冲区 */
	buf32_t buffer1 = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buffer1 == NULL) {
		printk(PART_ERROR "kmalloc for buffer1 failed!\n");
		return -1;
	}
	/* 2级块缓冲区 */
	buf32_t buffer2 = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buffer2 == NULL) {
		printk(PART_ERROR "kmalloc for buffer1 failed!\n");
		return -1;
	}
	/* 3级块缓冲区 */
	buf32_t buffer3 = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buffer3 == NULL) {
		printk(PART_ERROR "kmalloc for buffer1 failed!\n");
		return -1;
	}

	/* 1级间接块是指向节点文件块中的 */
	inderect1 = &inode->blocks[INDERCT_BLOCKS_LV3_INDEX];
	/* 如果1级间接块有数据 */
	if (*inderect1 != 0) {
		/* 读取1级间接块 */
		memset(buffer1, 0, sb->blockSize);
		
		if (BlockRead(sb->devno, *inderect1, buffer1)) {
			printk(PART_ERROR "device %d read failed!\n", sb->devno);
			goto ToEnd;
		}

		/* 2级间接块是指向1级间接块的数据中的 */
		inderect2 = &buffer1[idxInInderect1];
		
		/* 如果2级间接块有数据 */
		if (*inderect2 != 0) {
			/* 读取2级间接块 */
			memset(buffer2, 0, sb->blockSize);
			
			if (BlockRead(sb->devno, *inderect2, buffer2)) {
				printk(PART_ERROR "device %d read failed!\n", sb->devno);
				goto ToEnd;
			}

			/* 3级间接块是指向2级间接块的数据中的 */
			inderect3 = &buffer2[idxInInderect2];
			
			/* 如果2级间接块有数据 */
			if (*inderect3 != 0) {
				/* 读取3级间接块 */
				memset(buffer3, 0, sb->blockSize);
				
				if (BlockRead(sb->devno, *inderect3, buffer3)) {
					printk(PART_ERROR "device %d read failed!\n", sb->devno);
					goto ToEnd;
				}
				
				/* 直接块是指向3级间接块的数据中的 */
				derect = &buffer3[idxInInderect3];
				
				/* 直接块有数据 */
				if (*derect != 0) {

					if (FreeOneSector(sb, *derect)) {
						printk("free one block failed!\n");
						goto ToEnd;
					}

					//printk("%d ",*derect);

					#ifdef DEBUG_NODE_DATA
					printk("lv 3 free one block %d for derect.\n", *derect);
					printk("idx1 %d idx2 %d idx2 %d inderect1 %d inderect2 %d inderect3 %d derect %d\n",
						idxInInderect1, idxInInderect2, idxInInderect3, *inderect1, *inderect2, *inderect3, *derect);
					#endif			
					*derect = 0;

					/* 检测3级间接块是否是空块
					是空块，需要把这个间接块释放掉
					不是空块，就把间接块写回磁盘
					*/
					if (IsEmprtyBlock(buffer3, sb->blockSize)) {
						/* 释放3级间接块 */
						if (FreeOneSector(sb, *inderect3)) {
							printk("free one block failed!\n");
							goto ToEnd;
						}
						#ifdef DEBUG_NODE_DATA
						printk("lv 3 free one block %d for inderect3.\n", *inderect3);
						#endif
						*inderect3 = 0;
						
						/* 检测2级间接块是否是空块
						是空块，需要把这个间接块释放掉
						不是空块，就把间接块写回磁盘
						*/
						if (IsEmprtyBlock(buffer2, sb->blockSize)) {
							/* 释放2级间接块 */
							if (FreeOneSector(sb, *inderect2)) {
								printk("free one block failed!\n");
								goto ToEnd;
							}
							#ifdef DEBUG_NODE_DATA							
							printk("lv 3 free one block %d for inderect2.\n", *inderect2);
							#endif
							*inderect2 = 0;
								
							/* 检测1级间接块是否是空块
							是空块，需要把这个间接块释放掉
							不是空块，就把间接块写回磁盘
							*/
							if (IsEmprtyBlock(buffer1, sb->blockSize)) {
								/* 释放1级间接块 */
								if (FreeOneSector(sb, *inderect1)) {
									printk("free one block failed!\n");
									goto ToEnd;
								}
								#ifdef DEBUG_NODE_DATA
								printk("lv 3 free one block %d for inderect1.\n", *inderect1);
								printk("sync node when free blcok.\n");
						
								#endif
								*inderect1 = 0;
								
								/* 修改数据后同步到节点中去 */
								BOFS_SyncInode(inode, sb);
								//printk("sync node when free blcok.\n");
						
							} else {	
								/* 2级块修改之后，需要把2级块的值保存，也就是说，需要把
								2级块对应的数据写回磁盘，写回到1级间接块的数据中 */
								if (BlockWrite(sb->devno, *inderect1, buffer1, 0)) {
									printk(PART_ERROR "device %d write failed!\n", sb->devno);
									goto ToEnd;
								}
							}

						} else {	
							/* 3级块修改之后，需要把3级块的值保存，也就是说，需要把
							3级块对应的数据写回磁盘，写回到2级间接块的数据中 */
							if (BlockWrite(sb->devno, *inderect2, buffer2, 0)) {
								printk(PART_ERROR "device %d write failed!\n", sb->devno);
								goto ToEnd;
							}
						}
					} else {	
						/* 直接块修改之后，需要把直接块的值保存，也就是说，需要把
						直接块对应的数据写回磁盘，写回到3级间接块的数据中 */
						if (BlockWrite(sb->devno, *inderect3, buffer3, 0)) {
							printk(PART_ERROR "device %d write failed!\n", sb->devno);
							goto ToEnd;
						}
					}
				}
			}
		}		
	}
	/* 成功 */
	ret = 0;
	
ToEnd:
	kfree(buffer1);
	kfree(buffer2);
	kfree(buffer3);
	return ret;
}

/**
 * PutBlockByBlockIndex - 通过数据块索引释放块
 * @node: 节点文件
 * @index: 块索引
 * @sb: 超级块
 * 
 * 
 * 文件的大小依靠次函数的能力
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int PutBlockByBlockIndex(struct BOFS_Inode *inode,
	unsigned int index,
	struct BOFS_SuperBlock *sb)
{
	//printk("\n>>index %d->", index);
	/* 释放的思路，沿着找过去，是否存在，存在就把它释放。
	再检测所在块是否都释放了，如果是，就把该块也释放。
	以此类推，直到释放到node的blocks[]为止。释放一次需要把记录该
	数据的块同步到磁盘。如果是node的block[]，就需要同步node节点
	 */
	unsigned int base;
	
	/* 分级 */
	if (index < BLOCK_LV0) {
		/* 直接块，12个 */
		base = index;

		/* 可以直接获取直接块 */
		unsigned int *derect = &inode->blocks[base];
		/* 有数据块 */
		if (*derect != 0) {
			if (FreeOneSector(sb, *derect)) {
				printk("free one block failed!\n");
				return -1;
			}
			#ifdef DEBUG_NODE_DATA
			printk("level 0, index %d base %d derect %d\n", index, base, *derect);
			#endif
			//printk("level 0, index %d base %d derect %d\n", index, base, *derect);
			
			/* 把直接块置0 */
			*derect = 0;
			/* 修改数据后同步到节点中去 */
			BOFS_SyncInode(inode, sb);
		}
		//printk("level 0, index %d base %d derect %d\n", index, base, *derect);
	} else if (index < BLOCK_LV1) {
		/* 1级块,12 + 256个 */
		base = index - BLOCK_LV0;
		#ifdef DEBUG_NODE_DATA
		printk("level 1, index %d ", index);
		#endif
		//printk("level 1, index %d ", index);
		if (PutBlockInInderect1(inode, base, sb)) {
			return -1;
		}
	} else if (index < BLOCK_LV2) {
		/* 2级块,12 + 256 + 256 * 256个 */
		base = index - BLOCK_LV1;
		#ifdef DEBUG_NODE_DATA
		printk("level 2, index %d ", index);
		#endif
		if (PutBlockInInderect2(inode, base, sb)) {
			return -1;
		}
	} else if (index < BLOCK_LV3) {
		/* 3级块,12 + 256 + 256 * 256 + + 256 * 256 * 256个 */
		base = index - BLOCK_LV2;
		#ifdef DEBUG_NODE_DATA	
		printk("level 3, index %d ", index);
		#endif
		if (PutBlockInInderect3(inode, base, sb)) {
			return -1;
		}
	} else {
		printk(PART_ERROR "index %d out of boundary!\n");
		return -1;
	}
	return 0;
}

/**
 * BOFS_ReleaseInodeData - 释放所有节点中的数据
 * @sb: 超级块
 * @inode: 节点
 * 
 * 把所有逻辑块id对应的数据释放掉
 */
PUBLIC int BOFS_ReleaseInodeData(struct BOFS_SuperBlock *sb,
	struct BOFS_Inode *inode)
{
	/* 如果节点没有数据，就直接返回 */
	if (!inode->size)
		return 0;

	int index;

	unsigned int blocks = DIV_ROUND_UP(inode->size, sb->blockSize);
	
	//printk("node:%d size:%d blocks:%d\n",inode->id, inode->size, blocks);
	
	for(index = 0; index < blocks; index++){
		if (PutBlockByBlockIndex(inode, index, sb)) {
			printk("release node file failed!\n");
			return -1;
		}
	}
	return 0;
}
