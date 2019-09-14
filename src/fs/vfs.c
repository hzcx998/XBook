/*
 * file:		fs/vfs.c
 * auther:		Jason Hu
 * time:		2019/8/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/vfs.h>
#include <book/arch.h>
#include <book/slab.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/math.h>
#include <book/deviceio.h>
#include <book/vmalloc.h>
#include <book/vmarea.h>
#include <fs/absurd.h>
#include <fs/partition.h>

struct File globalFileTable[MAX_OPENED_FILE_NR];

PUBLIC int SysOpen(const char *path, unsigned int flags)
{
	int i;
	struct File *file = &globalFileTable[0];
	//printk(PART_TIP "SysOpen: path %s\n", path);
    
	for (i = 0; i < MAX_OPENED_FILE_NR; i++) {
		/* 名字一样*/
		if (!strcmp(file->name, (char *)path)) {
			file->flags = flags;
			file->pos = 0;
			return i;
		}
		file++;
	}
	printk(PART_ERROR "file open failed!\n");
	return -1;
}

PUBLIC int SysLseek(int fd, uint32_t offset, char flags)
{
	struct File *file = &globalFileTable[fd];
	if (flags == SEEK_SET) {
		//printk("set pos %x->%x\n", file->pos, offset);
		file->pos = offset;
	}
	return 0;
}

PUBLIC int SysRead(int fd, void *buffer, unsigned int size)
{
	struct File *file = &globalFileTable[fd];

	unsigned char *buf = file->start + file->pos;

	memcpy(buffer, buf, size);
	//printk("read data %x\n", *buf);
	return size;
}

PUBLIC int SysWrite(int fd, void *buffer, unsigned int size)
{
	struct File *file = &globalFileTable[fd];

	unsigned char *buf = file->start + file->pos;

	memcpy(buf, buffer, size);

	return size;
}

PUBLIC int SysClose(int fd)
{
	struct File *file = &globalFileTable[fd];
	file->pos = 0;
	return 0;
}

PUBLIC int SysStat(const char *path, struct Stat *buf)
{	
	int i;
	struct File *file = &globalFileTable[0];
	
	for (i = 0; i < MAX_OPENED_FILE_NR; i++) {
		/* 名字一样*/
		if (!strcmp(file->name, (char *)path)) {
			buf->st_size = file->size;

			return 0;
		}
		file++;
	}
	return -1;
}

PRIVATE void FileInit(int id, char *name, uint32_t addr, unsigned int size)
{
	struct File *file = &globalFileTable[id];
	memset(file->name, 0, MAX_FILE_NAMELEN);
	strcpy(file->name, name);
	file->start = (uint8_t *)addr;
	file->pos = 0;
	file->flags = 0;
	file->size = size;
	
}

#define DISK_COUNT_PER 30

PRIVATE unsigned int LoadDiskFile(unsigned char *fileBuf, int deiveceID, int lba, int count)
{
	unsigned char *diskBuf = kmalloc(SECTOR_SIZE * DISK_COUNT_PER, GFP_KERNEL);
	if (diskBuf == NULL)
		return -1; 
	
	int groups = DIV_ROUND_UP(count, DISK_COUNT_PER);
	printk("groups %d\n", groups);

	do {
		memset(diskBuf, 0, SECTOR_SIZE * DISK_COUNT_PER);
		
		if (DeviceRead(deiveceID, lba, diskBuf, DISK_COUNT_PER)) {
			printk("read failed!\n");
			return -1;
		}

		memcpy(fileBuf, diskBuf, SECTOR_SIZE * DISK_COUNT_PER);

		printk("%x-", *fileBuf);
		/* 文件指向下一个区域 */
		fileBuf += SECTOR_SIZE * DISK_COUNT_PER;
		lba += DISK_COUNT_PER;
		--groups;
	} while(groups > 0);

	kfree(diskBuf);
	return 0;
}


/**
 * InitVFS - 初始化虚拟文件系统
 * 
 * 虚拟文件系统的作用是，一个抽象的接口层，而不是做具体的文件管理
 * 操作，只是起到文件接口的统一，在这里进行抽象化。
 */
PUBLIC void InitVFS()
{
	PART_START("VFS");

	//unsigned char *fileBuf;

	FileInit(0, "init", 0x80042000, SECTOR_SIZE * 100);
	
	FileInit(1, "test", 0x80050000, SECTOR_SIZE * 100);

	FileInit(2, "shell", 0x80060000, SECTOR_SIZE * 100);
	/*
	fileBuf = (unsigned char *) 0xc0000000;
	LoadDiskFile(fileBuf, DEVICE_HDD0, 0, 600);
	FileInit(4, "sparrow", (unsigned int)fileBuf, 256*1024);
	
	fileBuf = (unsigned char *)kmalloc(SECTOR_SIZE, GFP_KERNEL);
	if (fileBuf == NULL) {
		Panic("filebuf null");
	}
	LoadDiskFile(fileBuf, DEVICE_HDD0, 1000, 1);
	FileInit(5, "sp", (unsigned int)fileBuf, 29);
	*/
	
	PART_END();
}

