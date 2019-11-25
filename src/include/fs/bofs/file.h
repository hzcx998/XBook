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

#define BOFS_MAX_FD_NR 32

#define BOFS_O_RDONLY 0x01 /*file open with read only*/
#define BOFS_O_WRONLY 0x02 /*file open with write only*/
#define BOFS_O_RDWR 0x04 /*file open with read and write*/
#define BOFS_O_CREAT 0x08 /*file open with create*/
#define BOFS_O_TRUNC 0x10 /*file open with trunc file size to 0*/
#define BOFS_O_APPEDN 0x20 /*file open with append pos to end*/
#define BOFS_O_EXEC 0x80 /*file open with execute*/

#define BOFS_SEEK_SET 1 	/*set a pos from 0*/
#define BOFS_SEEK_CUR 2		/*set a pos from cur pos*/
#define BOFS_SEEK_END 3		/*set a pos from end pos back*/

#define BOFS_F_OK 1		/*file existence*/
#define BOFS_X_OK 2		/*file executable*/
#define BOFS_R_OK 3		/*file readable*/
#define BOFS_W_OK 4		/*file writable*/

#define BOFS_FD_FREE 0
#define BOFS_FD_USING 0x80000000

/* 对文件的描述 */
struct BOFS_FileDescriptor
{
	unsigned int pos;	/*file cursor pos*/
	unsigned int flags; 		/*file operate flags*/
	
	struct BOFS_SuperBlock *superBlock;	/* 文件所在的超级块 */
	struct BOFS_DirEntry *dirEntry;		/* dir entry */
	struct BOFS_DirEntry *parentEntry;	/* parent dir entry */
	struct BOFS_Inode *inode;			/* file inode */
};

/* 记录一些重要信息 */
struct BOFS_Stat
{
	unsigned int type;
	unsigned int size;
	unsigned int mode;
	unsigned int device;
	
	/* 时间日期 */

};


void BOFS_InitFdTable();
int BOFS_AllocFdGlobal();
void BOFS_FreeFdGlobal(int fd);
struct BOFS_FileDescriptor *BOFS_GetFileByFD(int fd);

void BOFS_DumpFD(int fd);
PUBLIC int BOFS_Open(const char *pathname, unsigned int flags, struct BOFS_SuperBlock *sb);

int BOFS_Close(int fd);
PUBLIC int BOFS_Unlink(const char *pathname, struct BOFS_SuperBlock *sb);

int BOFS_Write(int fd, void* buf, unsigned int count);
int BOFS_Read(int fd, void* buf, unsigned int count);
int BOFS_Lseek(int fd, int offset, unsigned char whence);
int BOFS_Ioctl(int fd, int cmd, int arg);

PUBLIC int BOFS_Access(const char *pathname, int mode, struct BOFS_SuperBlock *sb);
PUBLIC int BOFS_GetMode(const char* pathname, struct BOFS_SuperBlock *sb);
PUBLIC int BOFS_SetMode(const char* pathname, int mode, struct BOFS_SuperBlock *sb);
PUBLIC int BOFS_Stat(const char *pathname,
    struct BOFS_Stat *buf,
    struct BOFS_SuperBlock *sb);

PUBLIC void BOFS_InitFile();

#endif

