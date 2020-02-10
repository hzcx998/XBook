/*
 * file:		include/fs/bofs/bitmap.h
 * auther:		Jason Hu
 * time:		2019/9/5
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BOFS_BITMAP_H
#define _BOFS_BITMAP_H

#include <lib/stdint.h>
#include <lib/types.h>
#include <fs/bofs/super_block.h>

/* 管理位图类型 */
enum BOFS_BM_TYPE
{
	BOFS_BMT_SECTOR,
	BOFS_BMT_INODE,
};

PUBLIC int BOFS_LoadBitmap(struct BOFS_SuperBlock *sb);
PUBLIC int BOFS_UnloadBitmap(struct BOFS_SuperBlock *sb);

PUBLIC int BOFS_AllocBitmap(struct BOFS_SuperBlock *sb, enum BOFS_BM_TYPE bmType, unsigned int counts);
PUBLIC int BOFS_FreeBitmap(struct BOFS_SuperBlock *sb, enum BOFS_BM_TYPE bmType, unsigned int idx);
PUBLIC int BOFS_SyncBitmap(struct BOFS_SuperBlock *sb, enum BOFS_BM_TYPE bmType, unsigned int idx);

/* 扇区位图idx和lba的转换 */
#define BOFS_IDX_TO_LBA(sb, idx) ((sb)->dataStartLba + (idx))
#define BOFS_LBA_TO_IDX(sb, lba) ((lba) - (sb)->dataStartLba)


#endif /* _BOFS_BITMAP_H */

