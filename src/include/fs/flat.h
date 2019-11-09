/*
 * file:		include/fs/flat.h
 * auther:		Jason Hu
 * time:		2019/10/25
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _FS_FLAT
#define _FS_FLAT

/*
FFS，平坦文件系统（Flat File System）
*/

#include <share/const.h>
#include <share/stddef.h>
#include <book/block.h>
#include <book/device.h>
#include <book/bitmap.h>

/* 路径长度 */
#define FLAT_MAGIC   0x19980325

/* 路径长度 */
#define PATH_NAME_LEN   128

/* 文件名长度 */
#define FILE_NAME_LEN   100

/* 目录名长度，-1是减去':'占用的空间 */
#define DIR_NAME_LEN    (PATH_NAME_LEN - FILE_NAME_LEN - 1)

PUBLIC int InitFlatFileSystem();

/* 路径解析 */
PUBLIC int ParseDirectoryName(char *path, char *buf);
PUBLIC int ParseFileName(char *path, char *buf);
PUBLIC struct SuperBlock *BuildFS(dev_t devno,
	sector_t start,
	sector_t count,
	size_t blockSize,
	size_t nodeNr);

#endif  /* _FS_FLAT */
