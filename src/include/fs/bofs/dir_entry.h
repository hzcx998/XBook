/*
 * file:		include/fs/bofs/dir_entry.h
 * auther:		Jason Hu
 * time:		2019/9/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOFS_DIR_ENTRY_H
#define _BOFS_DIR_ENTRY_H

#include <share/stdint.h>
#include <share/types.h>

/*book os file system dir entry*/

#define BOFS_NAME_LEN (256-6)

#define BOFS_FILE_TYPE_UNKNOWN 		0X00
#define BOFS_FILE_TYPE_NORMAL 		0X01    /* 普通文件 */
#define BOFS_FILE_TYPE_DIRECTORY 	0X02    /* 普通目录 */
#define BOFS_FILE_TYPE_TASK 	    0X04    /* 任务文件 */
#define BOFS_FILE_TYPE_BLOCK 	    0X08    /* 块设备 */
#define BOFS_FILE_TYPE_CHAR 	    0X10    /* 字符设备 */
#define BOFS_FILE_TYPE_NET 	        0X20    /* 网络设备 */
#define BOFS_FILE_TYPE_FIFO 	    0X40    /* 命名管道 */

#define BOFS_FILE_TYPE_INVALID 		0X8000

#define BOFS_PATH_LEN 	255

struct BOFS_DirEntry
{
	unsigned int inode;			/*inode number*/
	unsigned short type;	/*file type*/
	char name[BOFS_NAME_LEN];	/*name length(0~255)*/
}__attribute__ ((packed));;

#define BOFS_SIZEOF_DIR_ENTRY 	(sizeof(struct BOFS_DirEntry))

#define BOFS_DIR_NR_IN_SECTOR 	(BLOCK_SIZE/BOFS_SIZEOF_DIR_ENTRY)

PUBLIC void BOFS_CreateDirEntry(struct BOFS_DirEntry *dirEntry,
    unsigned int inode,
    unsigned short type,
    char *name);

PUBLIC void BOFS_DumpDirEntry(struct BOFS_DirEntry *dirEntry);

PUBLIC void BOFS_CloseDirEntry(struct BOFS_DirEntry *dirEntry);

#endif	/* _BOFS_DIR_ENTRY_H */

