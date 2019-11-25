/*
 * file:		include/fs/bofs/dir.h
 * auther:		Jason Hu
 * time:		2019/9/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOFS_DIR_H
#define _BOFS_DIR_H

#include <share/stdint.h>
#include <fs/bofs/dir_entry.h>

#define BOFS_BLOCK_SIZE 512

/*book os file system dir
 约定大小位48字节
*/
struct BOFS_Dir
{
	struct BOFS_DirEntry *dirEntry;
	unsigned int pos;
	unsigned int size;
	unsigned char *buf;

	/* 由于互相引用导致错误，这里就直接使用一个指针，使用的时候转换一下 */
	void *superBlock;	
	unsigned char pad[32];
}__attribute__ ((packed));

PUBLIC int BOFS_MountDir(const char *devpath, const char *pathname);
PUBLIC int BOFS_UnmountDir(const char *target);

PUBLIC void BOFS_DumpDir(struct BOFS_Dir* dir);

void BOFS_CloseDir(struct BOFS_Dir* dir);
PUBLIC struct BOFS_DirEntry *BOFS_ReadDir(struct BOFS_Dir *dir);
void BOFS_RewindDir(struct BOFS_Dir *dir);
void BOFS_ListDir(const char *pathname, int level);

int BOFS_PathToName(const char *pathname, char *namebuf);

int BOFS_GetCWD(char* buf, unsigned int size);
//int BOFS_ResetName(const char *pathname, char *name);
int BOFS_ChangeCWD(const char *pathname);

PUBLIC void BOFS_InitDir();


#endif

