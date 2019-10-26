/*
 * file:	    kernel/block/buffer.c
 * auther:	    Jason Hu
 * time:	    2019/10/13
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/debug.h>
#include <share/string.h>
#include <book/block.h>

#include <book/blk-buffer.h>
#include <book/blk-request.h>
#include <book/blk-disk.h>

EXTERN struct List allDiskList;

/**
 * LockBuffer - 锁定缓冲区
 * @bh: 缓冲头
 * 
 * 把缓冲区锁定，只有解锁后，调用者才能返回
 */
PUBLIC void LockBuffer(struct BufferHead *bh)
{
    bh->locked = 1;
}

/**
 * UnlockBuffer - 解除缓冲区的锁
 * @bh: 缓冲头
 * 
 * 解除缓冲区占用
 */
PUBLIC void UnlockBuffer(struct BufferHead *bh)
{
    bh->locked = 0;
}

/**
 * WaitOnBuffer - 在缓冲区上等待，直到缓冲区被解锁
 * @bh: 要等待的缓冲头
 * 
 */
PUBLIC void WaitOnBuffer(struct BufferHead *bh)
{
    while (bh->locked)
    {
        /* 让出CPU占用 */
        TaskYield();
    }
}

/**
 * GetBufferFromDisk - 从磁盘缓冲链表中获取一个缓冲
 * @disk: 要获取的磁盘
 * @lba: 块对应的lba
 * @size: 块的大小
 * 
 * 根据条件返回一个一样的块，没有找到则返回空
 */
PRIVATE struct BufferHead *GetBufferFromDisk(struct Disk *disk, sector_t lba, size_t size)
{
    struct BufferHead *bh = NULL, *tmp;

    /* 在磁盘对应的缓冲头中查找，是否和磁盘的设备号一致 */
    int devno = MKDEV(disk->major, disk->firstMinor);
    char flags = 0;
    ListForEachOwner(tmp , &disk->bufferHeadList, list) {
        /* 如果条件满足就找到 */
        if (tmp->devno == devno &&
            tmp->lba == lba &&
            tmp->size == size
        ) {
            bh = tmp;
            GetBH(bh);
            flags = 1;
            break;
        }
    }
    return bh;

    /* 找到了标志才为1，没有则返回空 */
    if (flags)
        return bh;
    else
        return NULL;
}

/**
 * CreateBuffers - 创建一个缓冲区
 * @disk: 缓冲区所属的磁盘（每个磁盘一组缓冲区）
 * @lba: 缓冲区的lba
 * @size: 缓冲区的大小
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int CreateBuffers(struct Disk *disk, sector_t lba, size_t size)
{
    if (size < SECTOR_SIZE || size > PAGE_SIZE)
        Panic("Bad block size!\n");

    /* 创建并添加到链表 */
    struct BufferHead *bh;

    char *data;

    data = kmalloc(size, GFP_KERNEL);
    if (data == NULL) {
        return -1;
    }
    bh = kmalloc(SIZEOF_BUFFER_HEAD, GFP_KERNEL);
    if (bh == NULL) {
        kfree(data);
        return -1;
    }

    AtomicSet(&bh->count, 0);
    bh->devno = MKDEV(disk->major, disk->firstMinor);
    bh->lba = lba;
    bh->size = size;
    bh->data = data;
    bh->private = NULL;

    bh->locked = 0;
    bh->uptodate = 0;
    bh->dirty = 0;
    
    /* 初始化信号量为1 */
    SemaphoreInit(&bh->sema, 1);

    /* 添加到buffer list */
    ListAddTail(&bh->list, &disk->bufferHeadList);

    return 0;
}

/**
 * GetBlock - 获取一个块
 * @devno: 设备号
 * @lba: 块号
 * 
 * 根据设备号获取一个块（缓冲区）
 */
PRIVATE struct BufferHead *GetBlock(dev_t devno, sector_t lba)
{
    struct BlockDevice *blkdev = GetBlockDeviceByDevno(devno);
    if (blkdev == NULL)
        return NULL;

    if (blkdev->disk == NULL)
        return NULL; 

    struct Disk *disk = blkdev->disk;

    while (1) {
        /* 从链表中获取块 */
        struct BufferHead *bh;
        bh = GetBufferFromDisk(disk, lba, blkdev->blockSize);
        
        /* 找到就返回 */
        if (bh != NULL) {
            return bh;
        }
        
        /* 没找到，就添加一个新的buffer */
        if (CreateBuffers(disk, lba, blkdev->blockSize)) {
            /* 如果添加新buffer失败，就尝试释放更多内存，再进行添加 */
            //FreeMoreMemory();
            Panic("Create block buffer failed!\n");
        }
    }
}

/**
 * Brelease - 释放一个块
 * @bh: 缓冲头
*/
PRIVATE void Brelease(struct BufferHead *bh)
{
    if (bh == NULL)
        return;
    
    /* 等待上锁的缓冲解锁 */
    WaitOnBuffer(bh);

    /* 减少引用计数 */
    PutBH(bh);
    if (!AtomicGet(&bh->count))
        Panic("Brelease: user count error!\n");

}

/**
 * RW_Block - 读/写一个块
 * @bh: 块缓冲头
 * 
 * 根据块缓冲头信息对块进行读写
 */
PRIVATE void RW_Block(int rw, struct BufferHead *bh)
{
    unsigned int major;

    major = MAJOR(bh->devno);

    MakeRequest(major, rw, bh);
}

/**
 * Bread - 读取一个块到缓冲头(Buffer Read)
 * @devno: 设备号
 * @block: 块号
 * 
 * 先获取一个块，然后检测数据是否有效，无效则从磁盘读取
 * 读取成功，则返回块缓冲头，失败返回空。
 */
PUBLIC struct BufferHead *Bread(dev_t devno, sector_t block)
{
    struct BufferHead *bh;
    //printk("Bread:start\n");
    if (!(bh = GetBlock(devno, block)))   // 获取一个buffer head
        Panic("Bread: Get block failed!\n");
    
    //printk("Bread:get a bh\n");
    //DumpBH(bh);
    
    // 是有效的，直接返回
    if (bh->uptodate) {
        //printk("is a uptodate bh.\n");
        return bh;
    }
    SemaphoreDown(&bh->sema);

    /* 没有就从磁盘读取数据到缓冲区 */
    RW_Block(BLOCK_READ, bh);

    WaitOnBuffer(bh);
    SemaphoreUp(&bh->sema);
    
    /* 读取数据之后，检测是否有数据 */
    if (bh->uptodate)
        return bh;
    
    printk("Bread: read block failed!");

    /* 获取读取数据失败，释放掉buffer head */
    Brelease(bh);
    return NULL;
}

/**
 * Bwrite - 写入一个块到缓冲头(Buffer Write)
 * @devno: 设备号
 * @block: 块号
 * @buffer: 写入的缓冲区
 * 
 * 直接把数据写入块缓冲，并设置“脏”位
 * 返回缓冲头，作为辅助信息
 */
PUBLIC struct BufferHead *Bwrite(dev_t devno, sector_t block, void *buffer)
{
    struct BufferHead *bh;
    //printk("Bwrite:start\n");
    if (!(bh = GetBlock(devno, block)))   // 获取一个buffer head
        Panic("Bwrite: Get block failed!\n");
    
    //printk("Bwrite:get a bh\n");
    //DumpBH(bh);
    SemaphoreDown(&bh->sema);
    
    /* 如果上锁了，就需要等待解锁 */
    WaitOnBuffer(bh);
    
    /* 把数据写入到缓冲头 */
    memcpy(bh->data, buffer, BLOCK_SIZE);

    /* 标记块为脏 */
    bh->dirty = 1;
    SemaphoreUp(&bh->sema);
    
    return bh;
}

/**
 * BsyncOne - 把块同步到磁盘(Buffer Sync One)
 * @bh: 块缓冲头
 * 
 * 把一个缓冲头对应的数据同步回磁盘
 * 成功返回1，失败返回0
 */
PUBLIC int BsyncOne(struct BufferHead *bh)
{
    /* 判断是否为脏 */
    if (!bh->dirty) {
        /* 不是脏缓冲，就直接返回 */
        return 0;
    }
    SemaphoreDown(&bh->sema);
    
    /* 没有就从磁盘读取数据到缓冲区 */
    RW_Block(BLOCK_WRITE, bh);

    WaitOnBuffer(bh);

    SemaphoreUp(&bh->sema);
    
    /* 写入数据之后，检测是为脏，不是脏，说写入成功 */
    if (!bh->dirty) {
        return 1;
    }
    printk("Bwrite: write block failed!");
    /* 获取读取数据失败，释放掉buffer head */
    Brelease(bh);
    return 0;
}

/**
 * Bsync - 同步所有脏缓冲(Buffer Sync)
 * 
 * 对所有磁盘中的所有缓冲进行一次同步
 */
PUBLIC int Bsync()
{
    struct Disk *disk = NULL;
    struct BufferHead *bh;
    
    /* 同步成功数 */
    int count = 0; 
    /* 获取磁盘 */
    ListForEachOwner(disk, &allDiskList, list) {
        bh = NULL;
        /* 获取磁盘中的的缓冲 */
        ListForEachOwner(bh ,&disk->bufferHeadList, list) {
            count += BsyncOne(bh);
        }
    }
    
    return count;
}

/**
 * DumpBH - 输出块缓冲头信息
 * @bh: 块缓冲头
 */
PUBLIC void DumpBH(struct BufferHead *bh)
{
    printk(PART_TIP "----Buffer Head----\n");
    printk(PART_TIP "devno:%x state:%d-%d-%d data:%x \n", bh->devno, bh->uptodate, bh->dirty, bh->locked, bh->data);
    printk(PART_TIP "device:%x private:%x\n", bh->device, bh->private);
    printk(PART_TIP "lba:%x size:%d count:%d \n", bh->lba, bh->size, bh->count);
}
