/*
 * file:		fs/vfs.c
 * auther:		Jason Hu
 * time:		2019/8/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/math.h>
#include <book/vmarea.h>
#include <fs/partition.h>
#include <fs/bofs/bofs.h>
#include <fs/bofs/file.h>
#include <fs/interface.h>
#include <book/blk-dev.h>

#include <fs/flat.h>
#include <fs/file.h>

PUBLIC int SysOpen(const char *path, unsigned int flags)
{
	return FlatOpen(path, flags);
}

PUBLIC int SysLseek(int fd, unsigned int offset, char flags)
{
	return FlatLseek(fd, offset, flags);
}

PUBLIC int SysRead(int fd, void *buffer, unsigned int size)
{
	return FlatRead(fd, buffer, size);
}

PUBLIC int SysWrite(int fd, void *buffer, unsigned int size)
{
	return FlatWrite(fd, buffer, size);
}

PUBLIC int SysClose(int fd)
{
	return FlatClose(fd);
}

PUBLIC int SysStat(const char *path, void *buf)
{	
	return FlatStat(path, buf);
}

PUBLIC int SysUnlink(const char *pathname)
{
	return 0;
}

PUBLIC int SysIoctl(int fd, int cmd, int arg)
{
	return 0;
}

PUBLIC int SysAccess(const char *pathname, int mode)
{
	return 0;
}

PUBLIC int SysGetMode(const char* pathname)
{
	return 0;
}

PUBLIC int SysSetMode(const char* pathname, int mode)
{
	return 0;
}

PUBLIC int SysMakeDir(const char *pathname)
{
	return 0;
}

PUBLIC int SysRemoveDir(const char *pathname)
{
	return 0;
}

PUBLIC int SysMountDir(const char *devpath, const char *dirpath)
{
	return 0;
}

PUBLIC int SysUnmountDir(const char *dirpath)
{
	return 0;
}

PUBLIC int SysGetCWD(char* buf, unsigned int size)
{
	return 0;
}

PUBLIC int SysChangeCWD(const char *pathname)
{
	return 0;
}

PUBLIC int SysChangeName(const char *pathname, char *name)
{
	return 0;
}

PUBLIC DIR *SysOpenDir(const char *pathname)
{
	
	return NULL;
}

PUBLIC void SysCloseDir(DIR *dir)
{
	
}

PUBLIC int SysReadDir(DIR *dir, DIRENT *buf)
{

	return 0;
}

PUBLIC void SysRewindDir(DIR *dir)
{
	
}

/* 同步磁盘上的数据到文件系统 */
#define SYNC_DISK_DATA

/* 数据是在软盘上的 */
#define DATA_ON_FLOPPY
#define FLOPPY_DATA_ADDR	0x80042000

/* 数据是在软盘上的 */
#define DATA_ON_IDE

/* 要写入文件系统的文件 */
#define FILE_ID 1

#if FILE_ID == 1
	#define FILE_NAME "root:init"
	#define FILE_SECTORS 40
#elif FILE_ID == 2
	#define FILE_NAME "root:shell"
	#define FILE_SECTORS 30
#elif FILE_ID == 3
	#define FILE_NAME "root:test"
	#define FILE_SECTORS 100
#endif

#define FILE_SIZE (FILE_SECTORS * SECTOR_SIZE)

PRIVATE void WriteDataToFS()
{
	#ifdef SYNC_DISK_DATA
	char *buf = kmalloc(FILE_SECTORS * SECTOR_SIZE, GFP_KERNEL);
	if (buf == NULL) {
		Panic("kmalloc for buf failed!\n");
	}
	
	/* 把文件加载到文件系统中来 */
	int fd = SysOpen(FILE_NAME, O_CREAT | O_RDWR);
    if (fd < 0) {
        printk("file open failed!\n");
		return;
    }
	
	memset(buf, 0, FILE_SECTORS * SECTOR_SIZE);
	
	/* 根据数据存在的不同形式选择不同的加载方式 */
	#ifdef DATA_ON_FLOPPY
		memcpy(buf, (void *)FLOPPY_DATA_ADDR, FILE_SIZE);
	#endif

	#ifdef DATA_ON_IDE


	#endif
	
	if (SysWrite(fd, buf, FILE_SIZE) != FILE_SIZE) {
		printk("write failed!\n");
	}

	SysLseek(fd, 0, SEEK_SET);
	if (SysRead(fd, buf, FILE_SIZE) != FILE_SIZE) {
		printk("read failed!\n");
	}

	kfree(buf);
	SysClose(fd);
	printk("load file sucess!\n");
	//Panic("test");
	#endif
}


#include <book/blk-disk.h>

/**
 * InitFileSystem - 初始化文件系统
 * 
 */
PUBLIC void InitFileSystem()
{
	PART_START("File System");

	
	InitFlatFileSystem();

	//Spin("----fs----");
	
	WriteDataToFS();
	
/*
	DIR *dir;
	DIRENT dirent;
	
	dir = SysOpenDir("/mnt/c");
	if (dir != NULL) {
		
		while (!SysReadDir(dir, &dirent)) {
			printk("dir name %s type %d inode %d size %d\n",
				dirent.name, dirent.type, dirent.inode, dirent.size);	
		}
	}*/
	/*
    int fd = SysOpen("root:test", O_CREAT | O_RDWR);
    if (fd < 0) {
        printk("file open failed!\n");
    }

    SysClose(fd);

	fd = SysOpen("root:test1", O_CREAT | O_RDWR);
    if (fd < 0) {
        printk("file open failed!\n");
    }

	char buf[32];
	memset(buf, 0x5a, 32); 
	if (SysWrite(fd, buf, 32) != 32) {
		printk("write failed!\n");
	}
	memset(buf, 0, 32);
	
	SysLseek(fd, 0, SEEK_SET);

	if (SysRead(fd, buf, 32) != 32) {
		printk("read failed!\n");
	}
	printk("%x %x\n", buf[0], buf[31]);
    SysClose(fd);*/

/*
	dir = SysOpenDir("/mnt/c");
	if (dir != NULL) {
		
		while (!SysReadDir(dir, &dirent)) {
			printk("dir name %s type %d inode %d size %d\n",
				dirent.name, dirent.type, dirent.inode, dirent.size);	
		}
	}*/
	/*
	if (SysRemoveDir("/mnt/c/dir")) {
		printk("remove dir failed!\n");
	}

	if (SysUnlink("/mnt/c/test")) {
		printk("unlink dir failed!\n");
	}*/

	PART_END();
}
