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
		file->pos = offset;
	}
	return 0;
}

PUBLIC int SysRead(int fd, void *buffer, unsigned int size)
{
	struct File *file = &globalFileTable[fd];

	unsigned char *buf = file->start + file->pos;

	memcpy(buffer, buf, size);

	return size;
}

PUBLIC int SysClose(int fd)
{
	return 0;
}
PRIVATE void FileInit(int id, char *name, uint32_t addr)
{
	struct File *file = &globalFileTable[id];
	memset(file->name, 0, MAX_FILE_NAMELEN);
	strcpy(file->name, name);
	file->start = (uint8_t *)addr;
	file->pos = 0;
	file->flags = 0;
}

PUBLIC void InitVFS()
{
	PART_START("VFS");
	FileInit(0, "init", 0x80042000);
	FileInit(1, "test", 0x80050000);
	FileInit(2, "shell", 0x80060000);
	/*
	int *a = (int *)0x80042000;
	int i;
	for (i = 0; i < 32; i++) {
		printk("%x ", a[i]);
	}
	
	int *b = (int *)0x80050000;

	for (i = 0; i < 32; i++) {
		printk("%x ", b[i]);
	}
	 */
	//Spin("InitVFS");
	PART_END();
}

