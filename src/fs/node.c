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
	memset(node, 0, SIZEOF_NODE_FILE);
	node->devno = devno;
	node->id = id;

	/* 设置时间信息 */
	node->crttime = SystemDateToData();
	node->acstime = node->mdftime = node->crttime;
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
	
	//printk("BOFS sync node: sec off:%d lba:%d buf off:%d\n", sectorOffset, lba, bufOffset);
	
	memset(sb->iobuf, 0, sb->blockSize);

    if (!BlockRead(sb->devno, lba, sb->iobuf)) {
        return -1;
    }
	struct NodeFile *node2 = (struct NodeFile *)sb->iobuf;
	*(node2 + bufOffset) = *node;
	
    if (!BlockWrite(sb->devno, lba, sb->iobuf, 0)) {
        return -1;
    }
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
