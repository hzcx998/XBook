/*
 * file:		fs/bofs/super_block.c
 * auther:		Jason Hu
 * time:		2019/9/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/string.h>
#include <fs/bofs/super_block.h>
#include <book/blk-buffer.h>
#include <share/math.h>
#include <fs/bofs/bitmap.h>

PUBLIC void BOFS_DumpSuperBlock(struct BOFS_SuperBlock *sb)
{
    printk(PART_TIP "----Super Block----\n");
    printk(PART_TIP "device id:%x sectors:%d sector bitmap lba:%d sector bitmap sectors:%d\n", 
        sb->devno, sb->totalSectors, sb->sectorBitmapLba, 
        sb->sectorBitmapSectors);
    printk(PART_TIP "inode bitmap lba:%d inode bitmap sectors:%d \
inode table lba:%d inode table sectors:%d data start lba:%d\n", 
        sb->inodeBitmapLba, sb->inodeBitmapSectors, 
        sb->inodeTableLba, sb->inodeTableSectors, 
        sb->dataStartLba);

    printk(PART_TIP "block size:%d\n", 
        sb->blockSize);
}

/**
 * BOFS_MountFS - 挂载文件系统
 * @superBlock: 超级块
 * 
 * 挂载文件系统，就是让文件系统的信息进入内存
 */
PUBLIC int BOFS_MountFS(struct BOFS_SuperBlock *superBlock)
{
    if (BOFS_LoadBitmap(superBlock)) {
        printk("BOFS load bitmap failed!\n");
        return -1;
    }

    /*
    if (BOFS_SuperBlockLoadBuffer(superBlock)) {
        printk("BOFS super block load buffer failed!\n");
        return -1;
    }*/

    return 0;
}

/**
 * BOFS_MakeFS - 创建文件系统
 * @devno: 设备号
 * @startSector: 起始扇区
 * @totalSectors: 扇区数
 * @metaIdCount: 元信息id数
 * 
 * 在一个磁盘的某个分区上创建一个文件系统，无条件创建，
 * 在进行格式化的时候就可以直接创建一个新的文件系统。
 * 
 * 成功: 返回超级块
 * 失败：返回NULL
 */
PUBLIC int BOFS_MakeFS(struct BOFS_SuperBlock *superBlock,
    dev_t devno, 
    sector_t startSector,
    size_t totalSectors,
    size_t blockSize,
    size_t inodeNr)
{
    printk(PART_TIP "fs info start sector %d sector count %d inode count %d\n",
        startSector, totalSectors, inodeNr);
    
    buf8_t iobuf;    /* 执行单次io需要 */
    buf8_t blkbuf; /* 执行大块io */
    
    /* 返回值默认是成功的 */
    int ret = 0;

    memset(superBlock, 0, blockSize);

    /*no fs on disk, format it*/
    printk(PART_TIP "Make a fs on %x.\n", devno);

    /* ----初始化超级块---- */
    
    superBlock->magic = BOFS_SUPER_BLOCK_MAGIC;
    superBlock->devno = devno;
    superBlock->blockSize = blockSize;
    
    superBlock->totalSectors = totalSectors;

    superBlock->maxInodes = inodeNr;
    superBlock->direntryNrInSector = blockSize / sizeof(struct BOFS_DirEntry);

    /* 超级块位于引导扇区后面 */
    superBlock->superBlockLba = startSector + 1;
    
    /* 设置位图长度 */
    superBlock->sectorBitmap.btmpBytesLen = superBlock->totalSectors / 8;
    superBlock->inodeBitmap.btmpBytesLen = inodeNr / 8;

    /* 位于超级块后面 */
    superBlock->sectorBitmapLba = superBlock->superBlockLba + 1;
    superBlock->sectorBitmapSectors = DIV_ROUND_UP(superBlock->sectorBitmap.btmpBytesLen, blockSize);
    
    superBlock->inodeBitmapLba = superBlock->sectorBitmapLba + superBlock->sectorBitmapSectors;
    superBlock->inodeBitmapSectors = DIV_ROUND_UP(superBlock->inodeBitmap.btmpBytesLen, blockSize);
    
    superBlock->inodeTableLba = superBlock->inodeBitmapLba + superBlock->inodeBitmapSectors;

    superBlock->inodeTableSectors = DIV_ROUND_UP(inodeNr * sizeof(struct BOFS_Inode), blockSize);
    
    superBlock->dataStartLba = superBlock->inodeTableLba + superBlock->inodeTableSectors;
    
    superBlock->rootInodeId = 0;
    superBlock->inodeNrInSector = blockSize/sizeof(struct BOFS_Inode);
    
    /* 
    设置扇区管理位图，inode管理位图，inode表，以及写入根目录
    */
    /*-----设置节点位图管理 -----*/
    iobuf = (buf8_t)kmalloc(blockSize, GFP_KERNEL);
    if (iobuf == NULL) {
        printk(PART_ERROR "kmalloc for iobuf failed!\n");
        ret = -1;
        goto ToEnd;
    }
    
    memset(iobuf, 0, blockSize);
    
    /* 使用1个inode，就是根目录的inode */
    iobuf[0] |= 1;
    
    if (BlockWrite(devno, superBlock->inodeBitmapLba, iobuf, 1)) {
        printk(PART_ERROR "device write failed!\n");
        ret = -1;
        goto ToFreeIoBuf;
    }

    /*-----创建根节点-----*/
    struct BOFS_Inode rootInode;
    BOFS_CreateInode(&rootInode, 0, 
        (BOFS_IMODE_D|BOFS_IMODE_R|BOFS_IMODE_W),
        0, devno);
    
    /* 设置数据扇区以及节点数据大小 */
    rootInode.blocks[0] = superBlock->dataStartLba;
    rootInode.size = sizeof(struct BOFS_DirEntry);
    
    memset(iobuf, 0, blockSize);
    memcpy(iobuf, &rootInode, sizeof(struct BOFS_Inode));
    
    if (BlockWrite(devno, superBlock->inodeTableLba, iobuf, 1)) {
        printk(PART_ERROR "device write failed!\n");
        ret = -1;
        goto ToFreeIoBuf;
    }

    //BOFS_DumpInode(&rootInode);

    /*-----创建根目录项-----*/
    
    struct BOFS_DirEntry rootDirEntry;

    BOFS_CreateDirEntry(&rootDirEntry, 0, BOFS_FILE_TYPE_DIRECTORY, 0, "/");

    memset(iobuf, 0, blockSize);
    memcpy(iobuf, &rootDirEntry, sizeof(struct BOFS_DirEntry));
    if (BlockWrite(devno, superBlock->dataStartLba, iobuf, 1)) {
        printk(PART_ERROR "device write failed!\n");
        ret = -1;
        goto ToFreeIoBuf;
    }

    /* 把使用的块从扇区管理中去掉 */
    sector_t usedBlocks = superBlock->dataStartLba - startSector;
    
    /* 数据块就使用了数据开始的第一个扇区 */
    usedBlocks++;

    unsigned int usedBytes = usedBlocks / 8;
    unsigned char usedBits = usedBlocks % 8;
    
    /* 缓冲区的块的数量 */
    unsigned int bufBlocks = DIV_ROUND_UP(usedBytes + 1, blockSize);

    printk("used blocks:%d bytes:%d bits:%d bufBlocks:%d\n",
        usedBlocks, usedBytes, usedBits, bufBlocks);
    
    /* 分配缓冲区 */
    blkbuf = kmalloc(bufBlocks * blockSize, GFP_KERNEL);
    if (blkbuf == NULL) {
        printk("kmalloc for buf failed!\n");
        ret = -1;
        goto ToFreeIoBuf;
    }
    memset(blkbuf, 0, bufBlocks * blockSize);
    
    /* 设置使用了的字节数 */
    memset(blkbuf, 0xff, usedBytes);
    
    /* 设置使用了后的剩余位数 */
    if (usedBits != 0) {
        int i;
        for (i = 0; i < usedBits; i++) {
            blkbuf[usedBytes] |= (1 << i);
        }
        //printk("buf=%x\n", buf[usedBytes]);
    }

    /* 写入块位图 */
    unsigned char *p = blkbuf;
    sector_t pos = superBlock->sectorBitmapLba;
    while (bufBlocks > 0) {
        //printk("block write pos:%d buf:%x\n", pos, p);
        if (BlockWrite(devno, pos, p, 1)) {
            printk("block write for block bitmap failed!\n");
            ret = -1;
            goto ToFreeBlkbuf;
        }
        pos++;
        p += blockSize;
        bufBlocks--;
    };

    /* 写入超级块 */
    if (BlockWrite(devno, superBlock->superBlockLba, superBlock, 1)) {
        printk("block write for sb failed!\n");
        ret = -1;
        goto ToFreeBlkbuf;
    }
ToFreeBlkbuf:
    kfree(blkbuf);
ToFreeIoBuf:
    kfree(iobuf);
ToEnd:
    return ret;

    //BOFS_DumpSuperBlock(superBlock);
/*
    */
    /*
    int idx = BOFS_AllocBitmap(superBlock, BOFS_BMT_SECTOR, 1);
    printk("idx %d\n", idx);
    BOFS_FreeBitmap(superBlock, BOFS_BMT_SECTOR, idx);

    idx = BOFS_AllocBitmap(superBlock, BOFS_BMT_INODE, 1);
    printk("idx %d\n", idx);
    BOFS_FreeBitmap(superBlock, BOFS_BMT_INODE, idx);

    idx = BOFS_AllocBitmap(superBlock, BOFS_BMT_SECTOR, 1);
    printk("idx %d\n", idx);
    idx = BOFS_AllocBitmap(superBlock, BOFS_BMT_INODE, 1);
    printk("idx %d\n", idx);
    idx = BOFS_AllocBitmap(superBlock, BOFS_BMT_SECTOR, 1);
    printk("idx %d\n", idx);
    idx = BOFS_AllocBitmap(superBlock, BOFS_BMT_INODE, 1);
    printk("idx %d\n", idx);
    */
    /*printk("sb size %d\n", sizeof(struct BOFS_SuperBlock));
    printk("dir size %d\n", sizeof(struct BOFS_Dir));
    printk("dir entry size %d\n", sizeof(struct BOFS_DirEntry));
    printk("inode size %d\n", sizeof(struct BOFS_Inode));
    */
    //Spin("test");
/*
    if (BOFS_OpenRootDir(superBlock)) {
        printk("BOFS open root dir failed!\n");
        return NULL;
    }
*/
    /* 每次一个 */
   
    //return superBlock;
}
