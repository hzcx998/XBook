/*
 * file:		include/book/blk-buffer.h
 * auther:		Jason Hu
 * time:		2019/10/13
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_BLOCK_BUFFER_H
#define _BOOK_BLOCK_BUFFER_H

#include <share/types.h>
#include <book/atomic.h>
#include <book/list.h>
#include <book/semaphore.h>
#include <book/device.h>
#include <book/block.h>

#include <book/blk-dev.h>

/**
 * 缓冲头用来描述单次读写操作的缓冲信息
 * 需要记录，读写扇区的基本信息（lba），缓冲区的大小
 * 还需要记录缓冲的地址，还要有一个缓冲链表，指向所在的设备
 * 
 * 缓冲头是用于对单次磁盘块读写的描述
 */
struct BufferHead {
    struct List list;
    char uptodate;      // 读取了新的数据
    char dirty;         // 写入了新数据
    char locked;       // 上锁

    char *data;

    dev_t devno;        // 设备号
    sector_t lba;       // 起始块位置
    size_t size;        // 缓冲区大小

    struct BlockDevice *device; // 设备指针

    void *private;  // 私有数据
    Atomic_t count; // 使用者计数
    struct Semaphore sema;
};

#define SIZEOF_BUFFER_HEAD sizeof(struct BufferHead)

PUBLIC struct BufferHead *Bread(dev_t dev, sector_t block);
PUBLIC struct BufferHead *Bwrite(dev_t dev, sector_t block, void *buffer);
PUBLIC int BsyncOne(struct BufferHead *bh);
PUBLIC int Bsync();

PUBLIC void DumpBH(struct BufferHead *bh);

PUBLIC void LockBuffer(struct BufferHead *bh);
PUBLIC void UnlockBuffer(struct BufferHead *bh);


STATIC INLINE void GetBH(struct BufferHead *bh)
{
    AtomicInc(&bh->count);
}

STATIC INLINE void PutBH(struct BufferHead *bh)
{
    AtomicDec(&bh->count);
}

/**
 * BlockRead - 块设备读取
 * @devno: 设备号
 * @block: 块
 * @buffer: 读取到缓冲区
 * 
 * 成功返回读取的数据量，失败返回0
 */
STATIC INLINE int BlockRead(dev_t devno, sector_t block,void *buffer)
{
    struct BufferHead *bh = Bread(devno, block);
    if (bh) {
        memcpy(buffer, bh->data, bh->size);
        return bh->size;
    }
    return 0;
}

/**
 * BlockWrite - 块设备写入
 * @devno: 设备号
 * @block: 块
 * @buffer: 要写入的缓冲区
 * 
 * 成功返回读取的数据量，失败返回0
 */
STATIC INLINE int BlockWrite(dev_t devno, sector_t block, void *buffer, char sync)
{
    struct BufferHead *bh = Bwrite(devno, block, buffer);
    
    if (bh) {
        if (sync) 
            return BsyncOne(bh);
        return 1;
    }
    return 0;
}


#endif   /* _BOOK_BLOCK_BUFFER_H */
