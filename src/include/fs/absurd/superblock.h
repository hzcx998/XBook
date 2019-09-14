/*
 * file:		include/fs/absurd/superblock.h
 * auther:		Jason Hu
 * time:		2019/9/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _FS_ABSURD_SUPER_BLOCK
#define _FS_ABSURD_SUPER_BLOCK

#include <share/stdint.h>
#include <share/types.h>
#include <share/string.h>
#include <book/bitmap.h>
#include <fs/absurd/meta.h>

#define ABSURD_SUPER_BLOCK_MAGIC 0x19980325

/* 约定最大有8个超级块 */
#define MAX_SUPER_BLOCK_NR 8

struct SuperBlock {
    /* 超级块魔数 */
    unsigned int magic; 

    /* 扇区管理位图 */
    unsigned int sectorBitmapLba;
    unsigned int sectorBitmapSectors;
    
    /* meta id管理位图 */
    unsigned int metaIdBitmapLba;
    unsigned int metaIdBitmapSectors;
    
    /* 数据区管理位图 */
    unsigned int dataAreaLba;
    unsigned int dataAreaSectors;

    /* 下面几个成员只有在内存中才有用 */
    
    struct DirEntry *rootDirEntry;  /* 根目录指针*/

    struct Bitmap sectorBitmap;     /* 扇区位图 */
    
    struct Bitmap metaIdBitmap;     /* 元信息ID位图 */

    /* 内存成员结束 */

    /* 元信息，必须放在最后面 */
    struct MetaInfo meta;
};

PUBLIC int InitSuperBlockTable();
PUBLIC struct SuperBlock *AllocSuperBlock();
PUBLIC void FreeSuperBlock(struct SuperBlock *sb);


PUBLIC int LoadMetaIdBitmap(struct SuperBlock *sb);
PUBLIC int LoadSectorBitmap(struct SuperBlock *sb);

PUBLIC unsigned int AllocMetaId(struct SuperBlock *sb, int count);
PUBLIC void FreeMetaId(struct SuperBlock *sb, unsigned int id, int count);
PUBLIC unsigned int AllocSector(struct SuperBlock *sb, int count);
PUBLIC void FreeSector(struct SuperBlock *sb, unsigned int lba, int count);

#endif  //_FS_ABSURD_SUPER_BLOCK
