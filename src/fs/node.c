/*
 * file:		fs/node.c
 * auther:		Jason Hu
 * time:		2019/11/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/string.h>
#include <driver/ide.h>
#include <driver/clock.h>
#include <fs/bitmap.h>
#include <share/math.h>
#include <fs/file.h>
#include <book/blk-buffer.h>
#include <fs/directory.h>
#include <fs/super_block.h>
#include <fs/node.h>

//#define DEBUG_NODE_DATA

/**
 * CloseNodeFile - 关闭一个节点
 * @node: 要关闭的节点
 * 
 */
PUBLIC void CloseNodeFile(struct NodeFile *node)
{
	if(node != NULL){
		kfree(node);
	}
}

/**
 * NodeFileInit - 初始化一个节点
 * @node: 节点地址
 * @id: 节点id
 * @mode: 节点的模式
 * @flags: 节点的标志
 * @devno: 设备号（devno）
 * 
 * 在节点中填写基础数据
 */
void NodeFileInit(struct NodeFile *node,
	dev_t devno,
    unsigned int id)
{
	node->devno = devno;
	node->id = id;

	/* 设置时间信息 */
	node->crttime = SystemDateToData();
	node->acstime = node->mdftime = node->crttime;

	/* 设置数据为0 */
	int i;
	for (i = 0; i < NODE_FILE_BLOCK_PTR_NR; i++) {
		node->blocks[i] = 0;
	}

}

/**
 * DumpNodeFile - 调试输出节点
 * @node: 要输出的节点
 */
void DumpNodeFile(struct NodeFile *node)
{
    printk(PART_TIP "---- Node File ----\n");

    printk(PART_TIP "name:%s type:%d attr:%x size:%d\n",
        node->super.name, node->super.type, node->super.attr, node->super.size);

	printk(PART_TIP "devno:%x id:%d block:%d -%d -%d -%d\n",
        node->devno, node->id, node->blocks[0], node->blocks[12],
		node->blocks[13], node->blocks[14]);
	
    printk(PART_TIP "Date: %d/%d/%d",
        DATA16_TO_DATE_YEA(node->crttime >> 16),
        DATA16_TO_DATE_MON(node->crttime >> 16),
        DATA16_TO_DATE_DAY(node->crttime >> 16));
    printk(" %d:%d:%d\n",
        DATA16_TO_TIME_HOU(node->crttime),
        DATA16_TO_TIME_MIN(node->crttime),
        DATA16_TO_TIME_SEC(node->crttime));
}

/**
 * SyncNodeFile - 把节点同步到磁盘
 * @node: 要同步的节点
 * @sb: 节点所在的超级块
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int SyncNodeFile(struct NodeFile *node, struct SuperBlock *sb)
{
	off_t blockOffset = node->id / sb->inodeNrInBlock;
	unsigned int lba = sb->nodeTableLba + blockOffset;
	off_t bufOffset = node->id % sb->inodeNrInBlock;
	
	//printk("sync node: block off:%d lba:%d buf off:%d\n", blockOffset, lba, bufOffset);
	
	unsigned char *buf = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buf == NULL) {
		printk("kmalloc for sync node file failed!\n");
		return -1;
	}

	//DumpNodeFile(node);

	memset(buf, 0, sb->blockSize);

    if (!BlockRead(sb->devno, lba, buf)) {
		kfree(buf);
        return -1;
    }
	struct NodeFile *node2 = (struct NodeFile *)buf;
	*(node2 + bufOffset) = *node;
	//DumpNodeFile(node2 + bufOffset);

    if (!BlockWrite(sb->devno, lba, buf, 0)) {
		kfree(buf);
        return -1;
    }
	kfree(buf);
    return 0;
}

/**
 * CleanNodeFile - 清空节点的信息
 * @node: 节点
 * @sb: 节点所在的超级块
 * 
 * 从磁盘上吧节点的所有信息清空
 * 成功返回0，失败返回-1
 */
int CleanNodeFile(struct NodeFile *node, struct SuperBlock *sb)
{
	off_t blockOffset = node->id / sb->inodeNrInBlock;
	unsigned int lba = sb->nodeTableLba + blockOffset;
	off_t bufOffset = node->id % sb->inodeNrInBlock;
	
	//printk("bofs sync: sec off:%d lba:%d buf off:%d\n", sectorOffset, lba, bufOffset);
	
	memset(sb->iobuf, 0, sb->blockSize);
	
	if (!BlockRead(sb->devno, lba, sb->iobuf)) {
		return -1;
	}

	struct NodeFile *nodeBuf = (struct NodeFile *)sb->iobuf;

	memset(&nodeBuf[bufOffset], 0, sizeof(struct NodeFile));
	
	if (!BlockWrite(sb->devno, lba, sb->iobuf, 0)) {
		return -1;
	}

	return 0;
}

/**
 * LoadnodeByID - 通过node id 加载节点信息
 * @node: 储存节点信息
 * @id: 节点id
 * @sb: 所在的超级块
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int LoadnodeByID(struct NodeFile *node, 
    unsigned int id, 
    struct SuperBlock *sb)
{

	off_t blockOffset = id / sb->inodeNrInBlock;
	uint32 lba = sb->nodeTableLba + blockOffset;
	off_t bufOffset = id % sb->inodeNrInBlock;
	
	//printk("BOFS load node sec off:%d lba:%d buf off:%d\n", blockOffset, lba, bufOffset);
	//printk("BOFS node nr in sector %d\n", sb->nodeNrInSector);
	
	memset(sb->iobuf, 0, sb->blockSize);
	if (!BlockRead(sb->devno, lba, sb->iobuf)) {
        return -1;
    }

	struct NodeFile *node2 = (struct NodeFile *)sb->iobuf;
	*node = node2[bufOffset];
	//Dumpnode(node);
	return 0;
}

/**
 * Copynode - 复制节点
 * @dst: 目的节点
 * @src: 源节点
 * 
 * 复制节点，而不是节点的数据
 * 成功返回0，失败返回-1    
 */
PUBLIC void Copynode(struct NodeFile *dst, struct NodeFile *src)
{
	memset(dst, 0, sizeof(struct NodeFile));
	memcpy(dst, src, sizeof(struct NodeFile));
}

/**
 * CreateNodeFile - 创建一个节点文件
 * @name: 文件名
 * @attr: 属性
 * @sb: 超级块
 * 
 * 创建过程如下：
 * 如果磁盘上文件节点是下面这样分布的
 * 0	test1	node
 * 1	test1	invalid
 * 
 * 如果要创建test，那么当遇到0的时候，名字是test1，属性是node，其对应的位图一定是使用中的
 * 那么就会继续向后扫描，遇到1的时候，名字是test1，类型是invlaid，那么对应的位图就是空闲的
 * 就会把1的这个节点分配，并且修改节点信息以及节点属性，即1，test，node。如下：
 * 0	test1	node
 * 1	test	node
 * 
 */
struct NodeFile *CreateNodeFile(char *name,
	char attr, struct SuperBlock *sb)
{
	/* 分配节点和节点id, 创建节点 */
	
	/* 创建一个节点文件 */
	struct File *file = CreateFile(name, FILE_TYPE_NODE, attr);

	if (file == NULL) {
		return NULL;
	}
	//printk("create file!\n");
	/* 为节点分配内存 */
	struct NodeFile *node = (struct NodeFile *)file;

	/* 分配节点位图 */
	unsigned int nodeID = FlatAllocBitmap(sb, FLAT_BMT_NODE, 1); 
	if (nodeID == -1) {
		printk(PART_ERROR "alloc node bitmap failed!\n");
		kfree(node);
		return NULL;
	}
	/* 保存节点id */

	//printk("alloc node %d!\n", nodeID);
	
	/* 把节点id同步到磁盘 */
	FlatSyncBitmap(sb, FLAT_BMT_NODE, nodeID);
    
	/* 初始化节点的信息 */
	NodeFileInit(node, sb->devno, nodeID);

	//printk("init node!\n");
	
	/* 同步节点信息 */
	SyncNodeFile(node, sb);
	//printk("sync node!\n");
	
	/* 文件数变多 */
	sb->files++;
	if (sb->files > sb->highestFiles) {
		sb->highestFiles++;
	}

	/* 同步超级块 */
	SyncSuperBlock(sb);
	//printk("sync sb!\n");
	
	return node;
}

/**
 * GetNodeFileByName - 从磁盘中读取节点文件
 * @dir: 目录
 * @name: 文件名字
 * 
 * 搜索过程如下：
 * 如果磁盘上文件节点是下面这样分布的
 * 0	test1	node
 * 1	test1	invalid
 * 2	test2	node
 * 3	test3	invalid
 * 4	test3	node
 * 
 * 如果要搜索test3，那么当遇到3的时候，名字相等，但是类型是invalid无效的
 * 那么就会继续向后搜索，遇到4的时候，名字相等，类型也是node，那么就满足
 * 
 * 成功返回文件节点指针，失败返回NULL
 */
struct NodeFile *GetNodeFileByName(struct Directory *dir, char *name)
{
	unsigned int blockID = 0;
	int i;

	struct SuperBlock *sb = dir->sb;
	sector_t lba = sb->nodeTableLba;

	struct NodeFile *nodeFile = kmalloc(SIZEOF_NODE_FILE, GFP_KERNEL);
	if (nodeFile == NULL) {
		printk(PART_ERROR "kmalloc for node file failed!\n");
		return NULL;
	}

	buf8_t iobuf = kmalloc(sb->blockSize, GFP_KERNEL);
	if (iobuf == NULL) {
		printk(PART_ERROR "kmalloc for io buf failed!\n");
		kfree(nodeFile);
		return NULL;
	}

	size_t blocks = DIV_ROUND_UP(sb->highestFiles, sb->blockSize);
	while (blockID < blocks) {
		/* 读取块 */
		memset(iobuf, 0, sb->blockSize);
		if (!BlockRead(sb->devno, lba, iobuf)) {
			printk(PART_ERROR "device %d read failed!\n", sb->devno);

			/* 搜索失败 */
			goto ToFailed;
		}
		
		struct NodeFile *node = (struct NodeFile *)iobuf;

		/*scan a sector*/
		for(i = 0; i < sb->inodeNrInBlock; i++){
			/* 有名字才进行检测 */
			if(node[i].super.name[0] != '\0'){
				//printk("node file name ->%s , search for dir entry ->%s\n", node[i].super.name, dir->name);
				if(!strcmp(node[i].super.name, name) && node[i].super.type != FILE_TYPE_INVALID){
					/* 找到相同名字的文件 */
					memcpy(nodeFile, &node[i], SIZEOF_NODE_FILE);

					/* 释放io缓冲 */
					kfree(iobuf);
					return nodeFile;
				}
			} else {
				/* 搜索失败 */
				goto ToFailed;
			}
		}
		blockID++;
		lba++;
	}
ToFailed:
	/* 释放io缓冲 */
	kfree(iobuf);
	kfree(nodeFile);

	return NULL;
}

/**
 * LoseNodeFile - 丢失一个节点文件
 * @node: 节点
 * @sb: 节点所在的超级块
 * @depth: 丢失深度 (0表示只使节点失效，1表示还要使删除节点对应的数据
 * 
 * 让节点id对应的节点文件变成无效，如果深度为1，还会回收文件数据内容
 * 成功返回0，失败返回-1
 */
PUBLIC int LoseNodeFile(struct NodeFile *node, struct SuperBlock *sb, char depth)
{
	switch (depth)
	{
	case 1: /* 删除节点对应的数据 */
		if (ReleaseNodeFileData(sb, node)) {
			return -1;
		}
	case 0: /* 使节点无效 */
		node->super.type = FILE_TYPE_INVALID;
		break;
	default:
		break;
	}

	/* 同步回磁盘 */
	if (SyncNodeFile(node, sb)) {
		return -1;
	}

	/* 回收节点位图 */
	FlatFreeBitmap(sb, FLAT_BMT_NODE, node->id);
	FlatSyncBitmap(sb, FLAT_BMT_NODE, node->id);
    
	/* 文件数变少 */
	sb->files--;

	/* 同步超级块 */
	SyncSuperBlock(sb);

	return 0;
}

PRIVATE int AllocOneBlock(struct SuperBlock *sb, unsigned int *data)
{
	int idx = FlatAllocBitmap(sb, FLAT_BMT_BLOCK, 1);
	if(idx == -1){
		printk("alloc block bitmap failed!\n");
		return -1;
	}
	FlatSyncBitmap(sb, FLAT_BMT_BLOCK, idx);
	*data = FLAT_IDX_TO_BLOCK(sb, idx); 
	//printk("d%d ",*data);
	
	/* 清空块里面的数据 */
	buf8_t buffer = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buffer == NULL) {
		printk(PART_ERROR "kmalloc for buf failed!\n");
		return -1;
	}
	
	memset(buffer, 0, sb->blockSize);
	if (!BlockWrite(sb->devno, *data, buffer, 0)) {
		printk(PART_ERROR "device %d write failed!\n", sb->devno);
		return -1;
	}
	
	kfree(buffer);
	
	return 0;
}

PRIVATE int FreeOneBlock(struct SuperBlock *sb, unsigned int block)
{
	/* 清空块里面的数据 */
	/*buf8_t buffer = kmalloc(sb->blockSize, GFP_KERNEL);
	if (buffer == NULL) {
		printk(PART_ERROR "kmalloc for buf failed!\n");
		return -1;
	}
	
	memset(buffer, 0, sb->blockSize);
	if (!BlockWrite(sb->devno, block, buffer, 0)) {
		printk(PART_ERROR "device %d write failed!\n", sb->devno);
		return -1;
	}
	kfree(buffer);*/
	/*printk("<<\n");
	
	
	*/
	//printk("block %d start %d\n", block, sb->dataStartLba);

	/* 释放块位图 */
	int idx = FLAT_BLOCK_TO_IDX(sb, block);
	//printk("<< %d\n", idx);
	
	FlatFreeBitmap(sb, FLAT_BMT_BLOCK ,idx);
	//printk("<<\n");
	
	FlatSyncBitmap(sb, FLAT_BMT_BLOCK, idx);
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
PRIVATE int GetBlockInInderect1(struct NodeFile *node,
	unsigned int base,
	unsigned int *block,
	struct SuperBlock *sb)
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
	inderect1 = &node->blocks[INDERCT_BLOCKS_LV1_INDEX];
	/* 如果间接块没有数据 */
	if (*inderect1 == 0) {
		/* 分配一个数据块 */
		if (AllocOneBlock(sb, inderect1)) {
			printk(PART_ERROR "alloc one block failed!\n");
			goto ToEnd;
		}
		//printk("lv 1 alloc one block %d for inderect1.\n", *inderect1);
		/* 修改数据后同步节点文件 */
		SyncNodeFile(node, sb);
	}

	/* 读取1级间接块 */
	memset(buffer1, 0, sb->blockSize);

	if (!BlockRead(sb->devno, *inderect1, buffer1)) {
		printk(PART_ERROR "device %d read failed!\n", sb->devno);
		goto ToEnd;
	}

	/* 直接块 */
	derect = &buffer1[base];
	/* 如果直接块没有数据 */
	if (*derect == 0) {
		/* 分配一个数据块 */
		if (AllocOneBlock(sb, derect)) {
			printk("lv 1 alloc one block failed!\n");
			goto ToEnd;
		}
		//printk("lv 1 alloc one block %d for derect.\n", *derect);
		//printk("%d ",*derect);
	
		/* 直接块修改之后，需要把直接块的值保存，也就是说，需要把
		直接块对应的数据写回磁盘，写回到1级间接块的数据中 */
		if (!BlockWrite(sb->devno, *inderect1, buffer1, 0)) {
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
PRIVATE int GetBlockInInderect2(struct NodeFile *node,
	unsigned int base,
	unsigned int *block,
	struct SuperBlock *sb)
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
	inderect1 = &node->blocks[INDERCT_BLOCKS_LV2_INDEX];

	/* 如果间接块没有数据 */
	if (*inderect1 == 0) {
		/* 分配一个数据块 */
		if (AllocOneBlock(sb, inderect1)) {
			printk(PART_ERROR "alloc one block failed!\n");
			goto ToEnd;
		}
		//printk("lv 2 alloc one block %d for inderect1.\n", *inderect1);
		/* 修改数据后同步节点文件 */
		SyncNodeFile(node, sb);
	}
	
	memset(buffer1, 0, sb->blockSize);

	/* 读取1级间接块 */
	if (!BlockRead(sb->devno, *inderect1, buffer1)) {
		printk(PART_ERROR "device %d read failed!\n", sb->devno);
		goto ToEnd;
	}

	/* 2级间接块是指向1级间接块的数据中的 */
	inderect2 = &buffer1[idxInInderect1];

	/* 如果2级间接块没有数据 */
	if (*inderect2 == 0) {
		/* 分配一个数据块 */
		if (AllocOneBlock(sb, inderect2)) {
			printk(PART_ERROR "alloc one block failed!\n");
			goto ToEnd;
		}

		//printk("lv 2 alloc one block %d for inderect2.\n", *inderect2);
		
		/* 2级间接块块修改之后，需要把块的值保存，也就是说，需要把
		2级间接块对应的数据写回磁盘，写回到1级间接块的数据中 */
		if (!BlockWrite(sb->devno, *inderect1, buffer1, 0)) {
			printk(PART_ERROR "device %d write failed!\n", sb->devno);
			goto ToEnd;
		}
	}
	
	memset(buffer2, 0, sb->blockSize);

	/* 读取2级间接块的数据 */
	if (!BlockRead(sb->devno, *inderect2, buffer2)) {
		printk(PART_ERROR "device %d read failed!\n", sb->devno);
		goto ToEnd;
	}

	/* 获取直接块，根据2级间接块的索引，从2级间接块中获取数据 */
	derect = &buffer2[idxInInderect2];

	/* 如果直接块没有数据 */
	if (*derect == 0) {
		/* 创建一个数据块 */
		if (AllocOneBlock(sb, derect)) {
			printk("alloc one block failed!\n");
			goto ToEnd;
		}
		//printk("%d ",*derect);
	
		/* 直接块修改之后，需要把直接块的值保存，也就是说，需要把
		直接块对应的数据写回磁盘，写回到2级间接块的数据中 */
		if (!BlockWrite(sb->devno, *inderect2, buffer2, 0)) {
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
PRIVATE int GetBlockInInderect3(struct NodeFile *node,
	unsigned int base,
	unsigned int *block,
	struct SuperBlock *sb)
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
	inderect1 = &node->blocks[INDERCT_BLOCKS_LV3_INDEX];

	/* 如果间接块没有数据 */
	if (*inderect1 == 0) {
		/* 分配一个数据块 */
		if (AllocOneBlock(sb, inderect1)) {
			printk(PART_ERROR "alloc one block failed!\n");
			goto ToEnd;
		}
		//printk("lv 3 alloc one block %d for inderect1.\n", *inderect1);
		/* 修改数据后同步节点文件 */
		SyncNodeFile(node, sb);
	}

	memset(buffer1, 0, sb->blockSize);

	/* 读取1级间接块 */
	if (!BlockRead(sb->devno, *inderect1, buffer1)) {
		printk(PART_ERROR "device %d read failed!\n", sb->devno);
		goto ToEnd;
	}

	/* 2级间接块是指向1级间接块的数据中的 */
	inderect2 = &buffer1[idxInInderect1];

	/* 如果2级间接块没有数据 */
	if (*inderect2 == 0) {
		/* 分配一个数据块 */
		if (AllocOneBlock(sb, inderect2)) {
			printk(PART_ERROR "alloc one block failed!\n");
			goto ToEnd;
		}

		//printk("lv 3 alloc one block %d for inderect2.\n", *inderect2);
		
		/* 2级间接块块修改之后，需要把块的值保存，也就是说，需要把
		2级间接块对应的数据写回磁盘，写回到1级间接块的数据中 */
		if (!BlockWrite(sb->devno, *inderect1, buffer1, 0)) {
			printk(PART_ERROR "device %d write failed!\n", sb->devno);
			goto ToEnd;
		}
	}
	
	memset(buffer2, 0, sb->blockSize);

	/* 读取2级间接块的数据 */
	if (!BlockRead(sb->devno, *inderect2, buffer2)) {
		printk(PART_ERROR "device %d read failed!\n", sb->devno);
		goto ToEnd;
	}

	/* 3级间接块是指向2级间接块的数据中的 */
	inderect3 = &buffer2[idxInInderect2];

	/* 如果3级间接块没有数据 */
	if (*inderect3 == 0) {
		/* 分配一个数据块 */
		if (AllocOneBlock(sb, inderect3)) {
			printk(PART_ERROR "alloc one block failed!\n");
			goto ToEnd;
		}
/*
		printk("lv 3 alloc one block %d for inderect3.\n", *inderect3);
		*/
		/* 3级间接块块修改之后，需要把块的值保存，也就是说，需要把
		3级间接块对应的数据写回磁盘，写回到2级间接块的数据中 */
		if (!BlockWrite(sb->devno, *inderect2, buffer2, 0)) {
			printk(PART_ERROR "device %d write failed!\n", sb->devno);
			goto ToEnd;
		}
	}
	
	memset(buffer3, 0, sb->blockSize);

	/* 读取3级间接块的数据 */
	if (!BlockRead(sb->devno, *inderect3, buffer3)) {
		printk(PART_ERROR "device %d read failed!\n", sb->devno);
		goto ToEnd;
	}

	/* 获取直接块，根据3级间接块的索引，从3级间接块中获取数据 */
	derect = &buffer3[idxInInderect3];

	/* 如果直接块没有数据 */
	if (*derect == 0) {
		/* 创建一个数据块 */
		if (AllocOneBlock(sb, derect)) {
			printk("alloc one block failed!\n");
			goto ToEnd;
		}

		/* 直接块修改之后，需要把直接块的值保存，也就是说，需要把
		直接块对应的数据写回磁盘，写回到3级间接块的数据中 */
		if (!BlockWrite(sb->devno, *inderect3, buffer3, 0)) {
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
 * GetBlockByBlockIndex - 通过数据块索引获取块
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
PUBLIC int GetBlockByBlockIndex(struct NodeFile *node,
	unsigned int index,
	unsigned int *block,
	struct SuperBlock *sb)
{
	unsigned int base;
	
	/* 分级 */
	if (index < BLOCK_LV0) {
		/* 直接块，12个 */
		base = index;

		/* 可以直接获取直接块 */
		unsigned int *derect = &node->blocks[base];
		/* 没有数据块 */
		if (*derect == 0) {
			/* 创建一个数据块 */
			if (AllocOneBlock(sb, derect)) {
				printk("alloc one block failed!\n");
				return -1;
			}
			
			/* 修改数据后同步回磁盘 */
			SyncNodeFile(node, sb);
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
		if (GetBlockInInderect1(node, base, block, sb)) {
			return -1;
		}
	} else if (index < BLOCK_LV2) {
		/* 2级块,12 + 256 + 256 * 256个 */
		base = index - BLOCK_LV1;

		#ifdef DEBUG_NODE_DATA
		printk("level 2, index %d ", index);
		#endif
		if (GetBlockInInderect2(node, base, block, sb)) {
			return -1;
		}
	} else if (index < BLOCK_LV3) {
		/* 3级块,12 + 256 + 256 * 256 + + 256 * 256 * 256个 */
		base = index - BLOCK_LV2;
		
		#ifdef DEBUG_NODE_DATA
		printk("level 3, index %d ", index);
		#endif
		if (GetBlockInInderect3(node, base, block, sb)) {
			return -1;
		}
	} else {
		printk(PART_ERROR "index %d out of boundary!\n");
		return -1;
	}
	return 0;
}

PUBLIC int NodeFileWrite(struct FileDescriptor *pfd, void *buffer, size_t count)
{
	//printk("node file %s write, count %d\n", pfd->file->name,  count);

	/* 1.矫正位置 */
	if (pfd->pos > pfd->file->size) {
		pfd->pos = pfd->file->size;
	}
	/* 2.判断是否需要改变文件大小，也就是说文件是否增大 */
	char expandFileSize = 0;
	if (pfd->pos + count > pfd->file->size) {
		expandFileSize = 1;		/* 需要扩展文件大小 */
	}

	/* 3.做数据写入前的准备 */
	buf8_t src = (buf8_t )buffer;	/* 要进行写入的缓冲区 */
	size_t bytesWritten = 0;		/* 已经写入的数据量 */
	size_t bytesLeft = count;		/* 剩下的字节数 */
	unsigned int block;					/* 逻辑块地址 */
	size_t blockOffsetBytes;		/* 块偏移字节数 */
	size_t blockLeftBytes;			/* 块剩余字节数 */
	size_t chunkSize;				/* 每次操作的数据数 */
	
	char writeFirstBlock = 1;		/* 写入的是第一个块 */

	struct NodeFile *node = (struct NodeFile *)pfd->file;

	/* 通过pos位置求出第一个要操作的块的id */
	struct SuperBlock *sb = pfd->dir->sb;
	unsigned int index = pfd->pos / sb->blockSize;

	/* 分配一个iobuf */
	buf8_t iobuf = kmalloc(sb->blockSize, GFP_KERNEL);
	if (iobuf == NULL) {
		printk("kmalloc for io buf failed!\n");
		return -1;
	}

	/* 4.开始写入数据 */
	while (bytesWritten < count) {
		block = 0;
		//printk("%d ", index);
		/* 通过数据块id获取块lba */
		if (GetBlockByBlockIndex(node, index, &block, sb)) {
			printk("get block failed!\n");
			break;	/* 跳出循环 */
		}
		//printk("%d ", block);

		memset(iobuf, 0, sb->blockSize);

		/* pos在一个块中的偏移字节数 */
		blockOffsetBytes = pfd->pos % sb->blockSize;	
		
		/* 在一个块中剩余的数据量 */
		blockLeftBytes = sb->blockSize - blockOffsetBytes;
		
		/* 根据剩余字节数和一个块中的剩余数判断要操作的数据的大小 */
		chunkSize = bytesLeft < blockLeftBytes ? bytesLeft : blockLeftBytes;
		
		//printk("index %d block:%d off:%d left:%d chunk:%d\n", index, block, blockOffsetBytes, blockLeftBytes, chunkSize);

		/* 如果是写入的第一个块，那么需要先读取第一个块的原有数据，再对其进行修改 */
		if (writeFirstBlock) {
			if(!BlockRead(sb->devno, block, iobuf)) {
				break;	/* 跳出循环 */
			}
			writeFirstBlock = 0;
			//printk("first write, need to read old data!\n");
		}

		/* 复制数据到io缓冲区中 */
		memcpy(iobuf + blockOffsetBytes, src, chunkSize);
		
		if(!BlockWrite(sb->devno, block, iobuf, 0)) {
			break;	/* 跳出循环 */
		}

		/* 改变位置 */
		src += chunkSize;
		pfd->pos += chunkSize;   
		/* 如果是需要扩展文件大小，那么就需要修改文件大小和pos一致 */
		if(expandFileSize){
			pfd->file->size  = pfd->pos;    // 更新文件大小
		}
		bytesWritten += chunkSize;
		bytesLeft -= chunkSize;
		index++;
	}

	/* 5.数据写入完毕，修改节点的信息，并同步到磁盘 */
	SyncNodeFile(node, sb);
	DumpNodeFile(node);
	kfree(iobuf);
	return bytesWritten;
}

PUBLIC int NodeFileRead(struct FileDescriptor *pfd, void *buffer, size_t count)
{
	////printk("node file %s read\n", pfd->file->name);
	
	/* 1.通过位置检测读取的数据量 */
	size_t size = count, bytesLeft = count;
	//printk("pos:%d count:%d size:%d\n", pfd->pos, count, pfd->file->size);

	// 如果要读取的数据已经超过文件的大小
	if ((pfd->pos + count) >= pfd->file->size){

		/* 截取文件数据大小 */
		size = pfd->file->size - pfd->pos;
		bytesLeft = size;
		//printk("sizeLeft:%d\n", bytesLeft);
		
		/* 没有需要读取的数据，也就是读取到文件末尾了 */
		if (bytesLeft == 0) {
			return -1;
		}
	}

	/* 3.做数据写入前的准备 */
	buf8_t dst = (buf8_t )buffer;	/* 要进行读取的缓冲区 */
	size_t bytesRead = 0;			/* 已经读取的数据量 */
	unsigned int block;				/* 逻辑块地址 */
	size_t blockOffsetBytes;		/* 块偏移字节数 */
	size_t blockLeftBytes;			/* 块剩余字节数 */
	size_t chunkSize;				/* 每次操作的数据数 */
	
	struct NodeFile *node = (struct NodeFile *)pfd->file;

	/* 通过pos位置求出第一个要操作的块的id */
	struct SuperBlock *sb = pfd->dir->sb;
	unsigned int index = pfd->pos / sb->blockSize;

	/* 分配一个iobuf */
	buf8_t iobuf = kmalloc(sb->blockSize, GFP_KERNEL);
	if (iobuf == NULL) {
		printk("kmalloc for io buf failed!\n");
		return -1;
	}

	/* 3.开始读取数据 */
	while (bytesRead < count) {
		block = 0;
		/* 通过数据块id获取块lba */
		if (GetBlockByBlockIndex(node, index, &block, sb)) {
			printk("get block failed!\n");
			break;	/* 跳出循环 */
		}

		/* pos在一个块中的偏移字节数 */
		blockOffsetBytes = pfd->pos % sb->blockSize;	
		
		/* 在一个块中剩余的数据量 */
		blockLeftBytes = sb->blockSize - blockOffsetBytes;
		
		/* 根据剩余字节数和一个块中的剩余数判断要操作的数据的大小 */
		chunkSize = bytesLeft < blockLeftBytes ? bytesLeft : blockLeftBytes;
		
		//printk("index:%d block:%d off:%d left:%d chunk:%d\n", index, block, blockOffsetBytes, blockLeftBytes, chunkSize);
		
		memset(iobuf, 0, sb->blockSize);

		if(!BlockRead(sb->devno, block, iobuf)) {
			break;	/* 跳出循环 */
		}

		/* 复制io缓冲区数据到dst数据中 */
		memcpy(dst, iobuf + blockOffsetBytes, chunkSize);
		
		/* 改变位置 */
		dst += chunkSize;
		pfd->pos += chunkSize;   
		bytesRead += chunkSize;
		bytesLeft -= chunkSize;
		index++;
	}
	
	/* 释放iobuf并返回成功读取的字节数 */
	kfree(iobuf);
	return bytesRead;
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
PRIVATE int PutBlockInInderect1(struct NodeFile *node,
	unsigned int base,
	struct SuperBlock *sb)
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
	inderect1 = &node->blocks[INDERCT_BLOCKS_LV1_INDEX];
	/* 如果1级间接块有数据 */
	if (*inderect1 != 0) {
		//printk("\nhas data %d\n", node->blocks[INDERCT_BLOCKS_LV1_INDEX]);
	
		/* 读取1级间接块 */
		memset(buffer1, 0, sb->blockSize);
		
		if (!BlockRead(sb->devno, *inderect1, buffer1)) {
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
			
			if (FreeOneBlock(sb, *derect)) {
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
				if (FreeOneBlock(sb, *inderect1)) {
					printk("free one block failed!\n");
					goto ToEnd;
				}
				#ifdef DEBUG_NODE_DATA
				printk("lv 1 free one block %d for inderect1.\n", *inderect1);
				printk("sync node when free blcok.\n");
				
				#endif
				*inderect1 = 0;
				/* 修改数据后同步到节点中去 */
				SyncNodeFile(node, sb);
				//printk("sync node when free blcok.\n");
						
			} else {	
				/* 直接块修改之后，需要把直接块的值保存，也就是说，需要把
				直接块对应的数据写回磁盘，写回到1级间接块的数据中 */
				if (!BlockWrite(sb->devno, *inderect1, buffer1, 0)) {
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
PRIVATE int PutBlockInInderect2(struct NodeFile *node,
	unsigned int base,
	struct SuperBlock *sb)
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
	inderect1 = &node->blocks[INDERCT_BLOCKS_LV2_INDEX];
	//printk("v%d ",*inderect1);
	
	
	/* 如果1级间接块有数据 */
	if (*inderect1 != 0) {
		/* 读取1级间接块 */
		memset(buffer1, 0, sb->blockSize);
		
		if (!BlockRead(sb->devno, *inderect1, buffer1)) {
			printk(PART_ERROR "device %d read failed!\n", sb->devno);
			goto ToEnd;
		}
			
		/* 2级间接块是指向1级间接块的数据中的 */
		inderect2 = &buffer1[idxInInderect1];
		
		/* 如果2级间接块有数据 */
		if (*inderect2 != 0) {
			/* 读取2级间接块 */
			memset(buffer2, 0, sb->blockSize);
			
			if (!BlockRead(sb->devno, *inderect2, buffer2)) {
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
				
				if (FreeOneBlock(sb, *derect)) {
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
					if (FreeOneBlock(sb, *inderect2)) {
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
						if (FreeOneBlock(sb, *inderect1)) {
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
						SyncNodeFile(node, sb);
						
					} else {	
						/* 2级块修改之后，需要把2级块的值保存，也就是说，需要把
						2级块对应的数据写回磁盘，写回到1级间接块的数据中 */
						if (!BlockWrite(sb->devno, *inderect1, buffer1, 0)) {
							printk(PART_ERROR "device %d write failed!\n", sb->devno);
							goto ToEnd;
						}
					}
				} else {	
					/* 直接块修改之后，需要把直接块的值保存，也就是说，需要把
					直接块对应的数据写回磁盘，写回到2级间接块的数据中 */
					if (!BlockWrite(sb->devno, *inderect2, buffer2, 0)) {
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
PRIVATE int PutBlockInInderect3(struct NodeFile *node,
	unsigned int base,
	struct SuperBlock *sb)
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
	inderect1 = &node->blocks[INDERCT_BLOCKS_LV3_INDEX];
	/* 如果1级间接块有数据 */
	if (*inderect1 != 0) {
		/* 读取1级间接块 */
		memset(buffer1, 0, sb->blockSize);
		
		if (!BlockRead(sb->devno, *inderect1, buffer1)) {
			printk(PART_ERROR "device %d read failed!\n", sb->devno);
			goto ToEnd;
		}

		/* 2级间接块是指向1级间接块的数据中的 */
		inderect2 = &buffer1[idxInInderect1];
		
		/* 如果2级间接块有数据 */
		if (*inderect2 != 0) {
			/* 读取2级间接块 */
			memset(buffer2, 0, sb->blockSize);
			
			if (!BlockRead(sb->devno, *inderect2, buffer2)) {
				printk(PART_ERROR "device %d read failed!\n", sb->devno);
				goto ToEnd;
			}

			/* 3级间接块是指向2级间接块的数据中的 */
			inderect3 = &buffer2[idxInInderect2];
			
			/* 如果2级间接块有数据 */
			if (*inderect3 != 0) {
				/* 读取3级间接块 */
				memset(buffer3, 0, sb->blockSize);
				
				if (!BlockRead(sb->devno, *inderect3, buffer3)) {
					printk(PART_ERROR "device %d read failed!\n", sb->devno);
					goto ToEnd;
				}
				
				/* 直接块是指向3级间接块的数据中的 */
				derect = &buffer3[idxInInderect3];
				
				/* 直接块有数据 */
				if (*derect != 0) {

					if (FreeOneBlock(sb, *derect)) {
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
						if (FreeOneBlock(sb, *inderect3)) {
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
							if (FreeOneBlock(sb, *inderect2)) {
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
								if (FreeOneBlock(sb, *inderect1)) {
									printk("free one block failed!\n");
									goto ToEnd;
								}
								#ifdef DEBUG_NODE_DATA
								printk("lv 3 free one block %d for inderect1.\n", *inderect1);
								printk("sync node when free blcok.\n");
						
								#endif
								*inderect1 = 0;
								
								/* 修改数据后同步到节点中去 */
								SyncNodeFile(node, sb);
								//printk("sync node when free blcok.\n");
						
							} else {	
								/* 2级块修改之后，需要把2级块的值保存，也就是说，需要把
								2级块对应的数据写回磁盘，写回到1级间接块的数据中 */
								if (!BlockWrite(sb->devno, *inderect1, buffer1, 0)) {
									printk(PART_ERROR "device %d write failed!\n", sb->devno);
									goto ToEnd;
								}
							}

						} else {	
							/* 3级块修改之后，需要把3级块的值保存，也就是说，需要把
							3级块对应的数据写回磁盘，写回到2级间接块的数据中 */
							if (!BlockWrite(sb->devno, *inderect2, buffer2, 0)) {
								printk(PART_ERROR "device %d write failed!\n", sb->devno);
								goto ToEnd;
							}
						}
					} else {	
						/* 直接块修改之后，需要把直接块的值保存，也就是说，需要把
						直接块对应的数据写回磁盘，写回到3级间接块的数据中 */
						if (!BlockWrite(sb->devno, *inderect3, buffer3, 0)) {
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
PUBLIC int PutBlockByBlockIndex(struct NodeFile *node,
	unsigned int index,
	struct SuperBlock *sb)
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
		unsigned int *derect = &node->blocks[base];
		/* 有数据块 */
		if (*derect != 0) {
			if (FreeOneBlock(sb, *derect)) {
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
			SyncNodeFile(node, sb);
		}
		//printk("level 0, index %d base %d derect %d\n", index, base, *derect);
	} else if (index < BLOCK_LV1) {
		/* 1级块,12 + 256个 */
		base = index - BLOCK_LV0;
		#ifdef DEBUG_NODE_DATA
		printk("level 1, index %d ", index);
		#endif
		//printk("level 1, index %d ", index);
		if (PutBlockInInderect1(node, base, sb)) {
			return -1;
		}
	} else if (index < BLOCK_LV2) {
		/* 2级块,12 + 256 + 256 * 256个 */
		base = index - BLOCK_LV1;
		#ifdef DEBUG_NODE_DATA
		printk("level 2, index %d ", index);
		#endif
		if (PutBlockInInderect2(node, base, sb)) {
			return -1;
		}
	} else if (index < BLOCK_LV3) {
		/* 3级块,12 + 256 + 256 * 256 + + 256 * 256 * 256个 */
		base = index - BLOCK_LV2;
		#ifdef DEBUG_NODE_DATA	
		printk("level 3, index %d ", index);
		#endif
		if (PutBlockInInderect3(node, base, sb)) {
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
PUBLIC int ReleaseNodeFileData(struct SuperBlock *sb,
	struct NodeFile *node)
{
	/* 如果节点没有数据，就直接返回 */
	if (!node->super.size)
		return 0;

	int index;

	unsigned int blocks = DIV_ROUND_UP(node->super.size, sb->blockSize);
	
	printk("node:%d size:%d blocks:%d\n",node->id, node->super.size, blocks);
	
	for(index = 0; index < blocks; index++){
		if (PutBlockByBlockIndex(node, index, sb)) {
			printk("release node file failed!\n");
			return -1;
		}
	}
	return 0;
}
