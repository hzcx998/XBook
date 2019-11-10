/*
 * file:		fs/flat.c
 * auther:		Jason Hu
 * time:		2019/10/26
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/debug.h>
#include <book/share.h>
#include <book/device.h>
#include <fs/flat.h>
#include <fs/file.h>
#include <fs/directory.h>
#include <driver/keyboard.h>
#include <driver/ide.h>
#include <fs/super_block.h>
#include <fs/node.h>
#include <fs/device.h>
#include <book/vmarea.h>

/**
 * ParseDirectoryName - 解析目录名
 * @path: 路径
 * @buf: 缓冲区
 * 
 * 解析格式是 目录名:文件名，中间必须要有:
 */
PUBLIC int ParseDirectoryName(char *path, char *buf)
{
	if (path == NULL)
		return 0;
	char *pos = strchr(path, ':');
	//printk("strchr:%x\n", pos);
	if (pos == NULL) {
		//printk("path no :\n");
		return 0;
	}
	//printk("not null\n");
	
	int len = pos - path;

	/* 只有:符号 */
	if (len <= 1) {
		//printk("dir is empty!\n");
		return 0;
	}

	strncpy(buf, path, len - 1);
	return 1;
}

/**
 * ParseFileName - 解析文件名
 * @path: 路径
 * @buf: 缓冲区
 * 
 * 解析格式是 目录名:文件名，中间必须要有:
 */
PUBLIC int ParseFileName(char *path, char *buf)
{
	if (path == NULL)
		return 0;
	char *pos = strrchr(path, ':');
	//printk("strrchr:%x\n", pos);
	if (pos == NULL) {
		//printk("path no :\n");
		return 0;
	}
	//printk("not null\n");
	
	/* 只有一个:，就说明没有文件名 */
	if (strlen(pos) <= 1) {
		//printk("path no file name\n");
		return 0;
	}
	strcpy(buf, pos + 1);
	return 1;
}

/**
 * BuildFS - 构建一个文件系统
 * @blkdev: 块设备
 * 
 * 根据块设备提供的信息构建一个文件系统
 */
PUBLIC struct SuperBlock *BuildFS(dev_t devno,
	sector_t start,
	sector_t count,
	size_t blockSize,
	size_t nodeNr)
{
	printk("file system build info:\ndevno %x, start %d,count %d, blockSize %d, node number %d\n",
		devno, start, count, blockSize, nodeNr);
	/*
	DeviceOpen(DEV_HDA, 0);

	DeviceIoctl(DEV_HDA, IDE_IO_CLEAN, 0);
	*/
	/* 获取超级块 */
	struct SuperBlock *sb = kmalloc(blockSize, GFP_KERNEL);
	if (sb == NULL) {
		printk("kmalloc for super block failed");
		return NULL;
	}
	memset(sb, 0, blockSize);

	/* 超级块位于引导扇区后面 */
	if (BlockRead(devno, start + 1, sb) != blockSize) {
		printk("load sb block failed!\n");
		return NULL;
	}

	/* 如果没有文件系统，就格式化一个的分区 */
	if (sb->magic != 1) {
  		printk("will format a file system on partiton.\n");

		sb->magic = FLAT_MAGIC;
		sb->devno = devno;
		sb->totalBlocks = count / (blockSize / SECTOR_SIZE);
		sb->blockSize = blockSize;
		sb->maxInodes = nodeNr;

		/* 超级块位于引导扇区之后 */
		sb->superBlockLba = start + 1;

		/* 设置位图长度 */
		sb->blockBitmap.btmpBytesLen = sb->totalBlocks / 8;
		sb->nodeBitmap.btmpBytesLen = nodeNr / 8;

		/* 扇区位图信息 */
		sb->blockBitmapLba = sb->superBlockLba + 1;
		/* 1字节可以表示8个扇区的状态 */
		sb->blockBitmapBlocks = DIV_ROUND_UP(sb->blockBitmap.btmpBytesLen, blockSize);
		
		/* 节点位图信息 */
		sb->nodeBitmapLba = sb->blockBitmapLba + sb->blockBitmapBlocks;
		/* 1字节可以表示8个节点的状态 */
		sb->nodeBitmapBlocks = DIV_ROUND_UP(sb->nodeBitmap.btmpBytesLen, blockSize);
		
		/* 节点位图信息 */
		sb->nodeTableLba = sb->nodeBitmapLba + sb->nodeBitmapBlocks;
		/* 1字节可以表示8个节点的状态 */
		sb->nodeTableBlocks = DIV_ROUND_UP(nodeNr * SIZEOF_NODE_FILE, 8 * blockSize);
		
		/* 数据区开始 */
		sb->dataStartLba = sb->nodeTableLba + sb->nodeTableBlocks;

		/* 一个块可以容纳多少个节点 */
		sb->inodeNrInBlock = blockSize / SIZEOF_NODE_FILE;

		/* 把使用的块从扇区管理中去掉 */
		sector_t usedBlocks = sb->dataStartLba - start;
		
		unsigned int usedBytes = usedBlocks / 8;
		unsigned char usedBits = usedBlocks % 8;
		
		/* 缓冲区的块的数量 */
		unsigned int bufBlocks = DIV_ROUND_UP(usedBytes + 1, blockSize);

		printk("used blocks:%d bytes:%d bits:%d bufBlocks:%d\n",
			usedBlocks, usedBytes, usedBits, bufBlocks);
		
		/* 分配缓冲区 */
		unsigned char *buf = kmalloc(bufBlocks * blockSize, GFP_KERNEL);
		if (buf == NULL) {
			printk("kmalloc for buf failed!\n");
			return NULL;
		}
		memset(buf, 0, bufBlocks * blockSize);
		
		/* 设置使用了的字节数 */
		memset(buf, 0xff, usedBytes);
		
		/* 设置使用了后的剩余位数 */
		if (usedBits != 0) {
			int i;
			for (i = 0; i < usedBits; i++) {
				buf[usedBytes] |= (1 << i);
			}
			//printk("buf=%x\n", buf[usedBytes]);
		}

		/* 写入块位图 */
		unsigned char *p = buf;
		sector_t pos = sb->blockBitmapLba;
		while (bufBlocks > 0) {
			//printk("block write pos:%d buf:%x\n", pos, p);
			if (!BlockWrite(devno, pos, p, 1)) {
				printk("block write for block bitmap failed!\n");
				kfree(buf);
				return NULL;
			}
			pos++;
			p += blockSize;
			bufBlocks--;
		};
		
		/* 写入超级块 */
		if (!BlockWrite(devno, sb->superBlockLba, sb, 1)) {
			printk("block write for sb failed!\n");
			kfree(buf);
			kfree(sb);
			return NULL;
		}
		/* 释放缓冲区 */
		kfree(buf);
	} else {
		printk("file system has exist!\n");
	}

	/* 返回超级块 */
	return sb;
}

/**
 * InitFlatFileSystem - 初始化平坦文件系统
 */
PUBLIC int InitFlatFileSystem()
{
    PART_START("Flat File System");
	
	/* 初始化文件描述符 */
	InitFileDescriptor();

    struct Directory *dir =  CreateDirectory("dev", DIR_TYPE_DEVICE);
    if (dir == NULL)
        return -1;
    DumpDirectory(dir);
	/* 获取设备文件 */
	struct Device *device;
	ListForEachOwner (device, &allDeviceListHead, list) {
		/* 根据注册到系统中的设备，创建对应的设备文件 */
		MakeDeviceFile("dev", device->name, FILE_TYPE_DEVICE, device);
	}
	
	ListDirectory();

	/* 挂载根目录 */
	MountDirectory("dev:hda0", "root");
	
	ListDirectory();
	//Spin("test");
	
	//Spin("test");
	/*int fd = FlatOpen("root:test4", O_CREAT | O_RDWR);
	if (fd < 0) {
		printk("file open failed!\n");
	}
	

	struct FileDescriptor *pfd = GetFileByDescriptor(fd);
	struct NodeFile *node = (struct NodeFile *)pfd->file;*/
/*
	unsigned int block = 0;

	unsigned int index = 0;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);
	//printk("index %d block %d\n", index, block);

	index = 11;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);
	//printk("index %d block %d\n", index, block);

	index = 12;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);
	//printk("index %d block %d\n", index, block);

	index = 13;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);
	//printk("index %d block %d\n", index, block);
	
	index = 256;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);
	//printk("index %d block %d\n", index, block);
	
	index = 256 + 11;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);
	
	index = 256+12;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);
	//printk("index %d block %d\n", index, block);

	index = 256+12 + 1;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);
	
	index = 256+12 + 256;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);
	
	index = 256+12 + 256 + 1;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);
	
	index = 256+12 + 256*10 + 1;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);
	
	index = 12 + 256 + 256*256;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);
	
	index = 12 + 256 + 256*256 + 1;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);

	index = 12 + 256 + 256*256 + 256*10;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);
	
	index = 12 + 256 + 256*256 + 256*256;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);
	
	index = 12 + 256 + 256*256 + 256*256*256-1;
	GetBlockByBlockIndex(node, index, &block, pfd->dir->sb);

	DumpNodeFile(node);

	index = 0;
	PutBlockByBlockIndex(node, index, pfd->dir->sb);

	index = 12;
	PutBlockByBlockIndex(node, index, pfd->dir->sb);
	//printk("index %d block %d\n", index, block);
	
	//
	index = 13;
	PutBlockByBlockIndex(node, index, pfd->dir->sb);
	//printk("index %d block %d\n", index, block);
	
	index = 256;
	PutBlockByBlockIndex(node, index, pfd->dir->sb);
	//printk("index %d block %d\n", index, block);
	
	index = 256 + 11;
	PutBlockByBlockIndex(node, index, pfd->dir->sb);
	
	index = 256+12;
	PutBlockByBlockIndex(node, index, pfd->dir->sb);
	//printk("index %d block %d\n", index, block);
	printk("9\n");
	index = 256+12 + 1;
	PutBlockByBlockIndex(node, index, pfd->dir->sb);
	printk("8\n");
	index = 256+12 + 256;
	PutBlockByBlockIndex(node, index, pfd->dir->sb);
	printk("7\n");
	index = 256+12 + 256 + 1;
	PutBlockByBlockIndex(node, index, pfd->dir->sb);
	printk("6\n");
	index = 256+12 + 256*10 + 1;
	PutBlockByBlockIndex(node, index, pfd->dir->sb);
	printk("5\n");
	index = 12 + 256 + 256*256;
	PutBlockByBlockIndex(node, index, pfd->dir->sb);
	printk("4\n");
	index = 12 + 256 + 256*256 + 1;
	PutBlockByBlockIndex(node, index, pfd->dir->sb);
	printk("3\n");
	index = 12 + 256 + 256*256 + 256*10;
	PutBlockByBlockIndex(node, index, pfd->dir->sb);
	printk("2\n");
	index = 12 + 256 + 256*256 + 256*256*1;
	PutBlockByBlockIndex(node, index, pfd->dir->sb);
	printk("1\n");
	index = 12 + 256 + 256*256 + 256*256*256-1;
	PutBlockByBlockIndex(node, index, pfd->dir->sb);
	Spin("test");
	
	DumpFileDescriptor(fd);
	*/

	/*int i, block;
	for (i = 0; i < 12+256+256*1;i++) {
		GetBlockByBlockIndex(node, i, &block, pfd->dir->sb);
		
	}
	printk("get done!\n");
	for (i = 0; i < 12+256+256*1;i++) {
		PutBlockByBlockIndex(node, i, pfd->dir->sb);
	}*/
	
	//DumpNodeFile(node);
/*
	int sz = 1 * MB;

	char *buf = vmalloc(sz);

	memset(buf, 0, sz);
	strcpy(buf, "hello, world!");
	buf[strlen(buf)] = 0;
	//FlatLseek(fd, 0, SEEK_SET);

	int write = FlatWrite(fd, buf, sz);
	printk("write bytes %d blocks %d\n", write, write / 1024);

	memset(buf, 0,sz);

	FlatLseek(fd, 0, SEEK_SET);

	int read = FlatRead(fd, buf, sz);
	printk("read bytes %d\n", read);
	
	printk("buf %s\n", buf);

	FlatClose(fd);

	FlatRemove("root:test4");
*/
/*
	int write = FlatWrite(fd, buf, 32);
	printk("write bytes %d\n", write);
	//  ("test");

	FlatLseek(fd, 6, SEEK_SET);

	memset(buf, 0, PAGE_SIZE*2);
	
	int read = FlatRead(fd, buf, 26);
	printk("read bytes %d\n", read);
	printk("read string %s\n", buf);

	write = FlatWrite(fd, buf, 32);
	printk("write bytes %d\n", write);

	FlatLseek(fd, 32, SEEK_SET);
	read = FlatRead(fd, buf, 32);
	printk("read bytes %d\n", read);
	printk("read string %s\n", buf);
*/
	
		/*
	fd = FlatOpen("dev:hda0", O_RDWR);
	if (fd < 0) {
		printk("file open failed!\n");
	}
	DumpFileDescriptor(fd);

	FlatClose(fd);

	if (FlatRemove("dev:hda0"))
		printk("file remove failed!\n");
	
	if (FlatRemove("root:test5"))
		printk("file remove failed!\n");
	*/
	/*
	fd = FlatOpen("dev:sda", O_CREAT | O_RDWR);
	if (fd < 0) {
		printk("file open failed!\n");
	}
	DumpFileDescriptor(fd);

	FlatClose(fd);*/
	
	/*
	int fd = FlatOpen("dev:hda", O_RDWR);
	if (fd < 0) {
		printk("file open failed!\n");
	}

	DumpFileDescriptor(fd);

	char buf[1024];
	memset(buf, 0x12, 1024);
	int ret = FlatWrite(fd, buf, 2);
	
	FlatLseek(fd, 0, SEEK_SET);
	memset(buf, 0, 1024);
	ret = FlatRead(fd, buf, 2);

	printk("read count:%d", ret);

	printk("buf :%x %x %x %x", buf[0], buf[511], buf[512], buf[1023]);

	FlatClose(fd);

	
	fd = FlatOpen("dev:keyboard", O_RDWR);
	if (fd < 0) {
		printk("file open failed!\n");
	}

	DumpFileDescriptor(fd);
	
	int key;

	FlatIoctl(fd, KBD_IO_MODE, KBD_MODE_ASYNC);

	while (1) {
		if (1 == FlatRead(fd, &key, 1)) {
			printk("%c", key); 
		}
	};

	FlatClose(fd);*/

	/*
    MakeDeviceFile("dev:hda", FILE_TYPE_DEVICE, NULL);
	MakeDeviceFile("dev:hdb", FILE_TYPE_DEVICE, NULL);
	*/

	/*
	dir =  CreateDirectory("root", DIR_TYPE_DEVICE);
    if (dir == NULL)
        return -1;
    DumpDirectory(dir);
	
    MakeDeviceFile("root:hda", FILE_TYPE_DEVICE, NULL);
	MakeDeviceFile("root:hdb", FILE_TYPE_DEVICE, NULL);
	*/

	/*dir =  CreateDirectory("root", DIR_TYPE_DEVICE);
    if (dir == NULL)
        return -1;
    DumpDirectory(dir);
	MakeDeviceFile("root:hda", FILE_TYPE_DEVICE, NULL);
	MakeDeviceFile("root:hdb", FILE_TYPE_DEVICE, NULL);
	*/

    PART_END();
    //Spin("test");

	return 0;
}