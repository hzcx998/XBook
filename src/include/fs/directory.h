/*
 * file:		include/fs/directory.h
 * auther:		Jason Hu
 * time:		2019/10/26
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */


#ifndef _FS_DIRECTORY
#define _FS_DIRECTORY

#include <book/list.h>
#include <fs/flat.h>

enum {
    DIR_TYPE_DEVICE = 1,
    DIR_TYPE_MOUNT,
};

/**
 * 目录，用户管理文件
 */
struct Directory {
    struct List list;               /* 目录在所有目录中的链表 */
    char name[DIR_NAME_LEN];        /* 目录的名字 */
    char type;                      /* 目录的类型，设备目录和挂载目录 */

    /* 设备目录的信息 */
    struct List deviceListHead;         /* 如果是设备目录，那么就用一个链表来管理所有设备文件 */

    /* 挂载目录的信息 */ 
    
};
#define SIZEOF_DIRECTORY sizeof(struct Directory)

PUBLIC struct Directory *CreateDirectory(char *name, char type);
PUBLIC int DestoryDirectory(char *name);

PUBLIC struct Directory *GetDirectoryByName(char *name);
PUBLIC void DumpDirectory(struct Directory *dir);
PUBLIC void ListDirectory();

#endif  /* _FS_DIRECTORY */
