/*
 * file:		include/book/vfs.h
 * auther:		Jason Hu
 * time:		2019/8/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_VFS_H
#define _BOOK_VFS_H

#include <share/types.h>
#include <share/const.h>
#include <share/stddef.h>
#include <book/arch.h>

#define MAX_OPENED_FILE_NR 6

#define MAX_FILE_NAMELEN 32

#define SEEK_SET 1

#define O_RDONLY 0x01

struct File {
    uint32_t pos;
    uint8_t *start;
    uint32_t flags;
    unsigned int size;
    char name[MAX_FILE_NAMELEN];
};

struct Stat {
    unsigned int st_size;
};

PUBLIC void InitVFS();
PUBLIC int SysOpen(const char *path, unsigned int flags);
PUBLIC int SysRead(int fd, void *buffer, unsigned int size);
PUBLIC int SysWrite(int fd, void *buffer, unsigned int size);
PUBLIC int SysLseek(int fd, uint32_t offset, char flags);
PUBLIC int SysStat(const char *path, struct Stat *buf);
PUBLIC int SysClose(int fd);

#endif   /* _BOOK_VFS_H */
