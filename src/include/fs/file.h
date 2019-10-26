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
#include <share/file.h>
#include <share/dir.h>

typedef unsigned int loff_t;

struct File {
    struct FileOperations *opSets;  /* 文件操作集合 */
};

struct FileOperations {
    loff_t (*lseek)(struct File *, loff_t, int);
    size_t (*read)(struct File *, char *, size_t, loff_t *);
    size_t (*write)(struct File *, char *, size_t, loff_t *);
    int (*open)(struct File *);
    int (*release)(struct File *);
    int (*fsync)(struct File *, int sync); 
};

#endif  /* _FS_FILE */
