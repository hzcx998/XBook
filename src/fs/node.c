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
	for (i = 0; i < NODE_FILE_BLOCK_PRT_NR; i++) {
		node->block[i] = 0;
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
        node->devno, node->id, node->block[0], node->block[12],
		node->block[13], node->block[14]);
	
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
	
	printk("sync node: block off:%d lba:%d buf off:%d\n", blockOffset, lba, bufOffset);
	
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
 * 
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

	bbuf_t iobuf = kmalloc(sb->blockSize, GFP_KERNEL);
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
				printk("node file name ->%s , search for dir entry ->%s\n", node[i].super.name, dir->name);
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
