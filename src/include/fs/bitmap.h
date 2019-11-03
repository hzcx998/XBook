/*
 * file:		include/fs/bitmap.h
 * auther:		Jason Hu
 * time:		2019/11/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _FS_BITMAP_H
#define _FS_BITMAP_H

#include <share/stdint.h>
#include <share/types.h>
#include <fs/flat.h>

/* 管理位图类型 */
enum FLAT_BM_TYPE
{
	FLAT_BMT_BLOCK,
	FLAT_BMT_NODE,
};

PUBLIC int LoadSuperBlockBitmap(struct SuperBlock *sb);
int FlatAllocBitmap(struct SuperBlock *sb, enum FLAT_BM_TYPE bmType, size_t counts);
int FlatFreeBitmap(struct SuperBlock *sb, enum FLAT_BM_TYPE bmType, unsigned int idx);
int FlatSyncBitmap(struct SuperBlock *sb, enum FLAT_BM_TYPE bmType, unsigned int idx);

/* 块位图idx和lba的转换 */
#define FLAT_IDX_TO_LBA(sb, idx) ((sb)->dataStartLba + (idx))
#define FLAT_LBA_TO_IDX(sb, lba) ((lba) - (sb)->dataStartLba)

#endif /* _FS_BITMAP_H */

