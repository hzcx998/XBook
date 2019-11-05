/*
 * file:		include/fs/file.h
 * auther:		Jason Hu
 * time:		2019/10/25
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _FS_FILE
#define _FS_FILE

#include <share/const.h>
#include <share/stddef.h>
#include <book/device.h>

#include <fs/directory.h>

enum {
    FILE_TYPE_INVALID = 0,
    FILE_TYPE_DEVICE,
    FILE_TYPE_NODE,
};

#define FILE_ATTR_R     0x01
#define FILE_ATTR_W     0x02
#define FILE_ATTR_X     0x04

/**
 * 文件
 * size:106
 */
struct File {
    char name[FILE_NAME_LEN];       /* 文件的名字 */
    char type;                      /* 文件的类型 */
    char attr;                      /* 文件的属性 */
    size_t size;                    /* 文件的大小 */
} __attribute__ ((packed));

#define SIZEOF_FILE sizeof(struct File)

/* 系统中最多可以打开的文件数量 */
#define MAX_OPEN_FILE_NR    64 

PUBLIC struct File *CreateFile(char *name, char type, char attr);
PUBLIC int IsFileEqual(struct File *file1, struct File *file2);

/* 最高位置1 */
#define FILE_FLAGS_USNING    (1 << 31)

/**
 * 文件描述符
 * 用于描述一个文件打开的信息
 */
struct FileDescriptor {
    struct File *file;      /* 描述的文件 */
    unsigned int pos;	    /* 文件读写位置 */
	unsigned int flags; 	/* 文件操作标志 */
	struct Directory *dir;  /* 文件所在的目录 */
};

PUBLIC void InitFileDescriptor();
void DumpFileDescriptor(int fd);

PUBLIC int FlatOpen(const char *path, int flags);
PUBLIC int FlatClose(int fd);
PUBLIC int FlatWrite(int fd, void* buf, size_t count);
PUBLIC int FlatRead(int fd, void* buf, size_t count);
PUBLIC off_t FlatLseek(int fd, off_t offset, char whence);
PUBLIC int FlatIoctl(int fd, unsigned int cmd, unsigned int arg);
PUBLIC int FlatRemove(const char *path);

#endif  /* _FS_FILE */
