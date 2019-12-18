/*
 * file:		include/fs/bofs/file.h
 * auther:		Jason Hu
 * time:		2019/9/17
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOFS_FILE_H
#define _BOFS_FILE_H

#include <share/stdint.h>
#include <share/types.h>
#include <fs/bofs/super_block.h>
#include <book/task.h>

#define BOFS_MAX_FD_NR 128

#define BOFS_O_RDONLY 0x01 /*file open with read only*/
#define BOFS_O_WRONLY 0x02 /*file open with write only*/
#define BOFS_O_RDWR 0x04 /*file open with read and write*/
#define BOFS_O_CREAT 0x08 /*file open with create*/
#define BOFS_O_TRUNC 0x10 /*file open with trunc file size to 0*/
#define BOFS_O_APPEDN 0x20 /*file open with append pos to end*/
#define BOFS_O_EXEC 0x80 /*file open with execute*/

#define BOFS_FLAGS_PIPE 0x100   /* 管道文件标志 */

#define BOFS_SEEK_SET 1 	/*set a pos from 0*/
#define BOFS_SEEK_CUR 2		/*set a pos from cur pos*/
#define BOFS_SEEK_END 3		/*set a pos from end pos back*/

#define BOFS_F_OK 1		/*file existence*/
#define BOFS_X_OK 2		/*file executable*/
#define BOFS_R_OK 3		/*file readable*/
#define BOFS_W_OK 4		/*file writable*/

#define BOFS_FD_FREE 0
#define BOFS_FD_USING 0x80000000

/* 标准输入输出描述符 */
enum {
   STDIN_FD,   // 0 标准输入
   STDOUT_FD,  // 1 标准输出
   STDERR_FD   // 2 标准错误
};

/* 对文件的描述 */
struct BOFS_FileDescriptor
{
	unsigned int pos;	/*file cursor pos*/
	unsigned int flags; 		/*file operate flags*/
	
    Atomic_t reference; /* 引用 */
	struct BOFS_SuperBlock *superBlock;	/* 文件所在的超级块 */
	struct BOFS_DirEntry *dirEntry;		/* dir entry */
	struct BOFS_DirEntry *parentEntry;	/* parent dir entry */
	struct BOFS_Inode *inode;			/* file inode */
};

/* 记录一些重要信息 */
struct BOFS_Stat
{
	size_t size;            /* 以字节为单位的文件大小 */
	mode_t mode;            /* 模式 */
	dev_t devno;            /* 设备号 */
    dev_t devno2;           /* 设备文件指向的设备号 */
    
    ino_t inode;            /* 索引节点号 */
    
	/* 时间日期 */
	time_t crttime;	        /* 创建时间 */
	time_t mdftime;	        /* 修改时间 */
	time_t acstime;	        /* 访问时间 */
	
    blksize_t blksize;      /* 单个块的大小 */
	blkcnt_t blocks;        /* 文件占用的磁盘块数 */
};

PUBLIC void BOFS_InitFdTable();
PUBLIC int BOFS_AllocFdGlobal();
PUBLIC void BOFS_FreeFdGlobal(int fd);
PUBLIC struct BOFS_FileDescriptor *BOFS_GetFileByFD(int fd);
PUBLIC void BOFS_GlobalFdAddFlags(unsigned int globalFd, unsigned int flags);
PUBLIC void BOFS_GlobalFdDelFlags(unsigned int globalFd, unsigned int flags);
PUBLIC int BOFS_GlobalFdHasFlags(unsigned int globalFd, unsigned int flags);
PUBLIC uint32_t FdLocal2Global(uint32_t localFD);
PUBLIC int TaskInstallFD(int globalFdIdx);


PUBLIC void BOFS_DumpFD(int fd);
PUBLIC int BOFS_Open(const char *pathname, unsigned int flags, struct BOFS_SuperBlock *sb);

PUBLIC int BOFS_Close(int fd);
PUBLIC int BOFS_Remove(const char *pathname, struct BOFS_SuperBlock *sb);

PUBLIC int BOFS_Write(int fd, void *buf, unsigned int count);
PUBLIC int BOFS_Read(int fd, void *buf, unsigned int count);
PUBLIC int BOFS_Lseek(int fd, int offset, unsigned char whence);
PUBLIC int BOFS_Ioctl(int fd, int cmd, int arg);

PUBLIC int BOFS_Fsync(int fd);

PUBLIC int BOFS_Stat(const char *pathname,
    struct BOFS_Stat *buf,
    struct BOFS_SuperBlock *sb);

PUBLIC void BOFS_InitFile();

PUBLIC void BOFS_UpdateInodeOpenCounts(struct Task *task);
PUBLIC void BOFS_ReleaseTaskFiles(struct Task *task);

#endif

