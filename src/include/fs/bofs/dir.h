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
	unsigned char pad[32];
}__attribute__ ((packed));

PUBLIC int BOFS_MakeDir(const char *pathname);
PUBLIC int BOFS_RemoveDir(const char *pathname);

PUBLIC int BOFS_MountDir(const char *pathname, char *devname);

PUBLIC void BOFS_DumpDir(struct BOFS_Dir* dir);

struct BOFS_Dir *BOFS_OpenDir(const char *pathname);
void BOFS_CloseDir(struct BOFS_Dir* dir);
PUBLIC struct BOFS_DirEntry *BOFS_ReadDir(struct BOFS_Dir *dir);
void BOFS_RewindDir(struct BOFS_Dir *dir);
void BOFS_ListDir(const char *pathname, int level);

#endif

