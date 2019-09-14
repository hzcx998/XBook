/*
 * file:		fs/bofs/main.c
 * auther:		Jason Hu
 * time:		2019/9/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/slab.h>
#include <book/debug.h>
#include <share/string.h>
#include <book/deviceio.h>
#include <share/string.h>
#include <driver/ide.h>
#include <fs/partition.h>
#include <fs/bofs/super_block.h>
#include <fs/bofs/inode.h>
#include <fs/bofs/dir_entry.h>
#include <fs/bofs/bitmap.h>
#include <fs/bofs/dir.h>

/**
 * BOFS_MountMasterFS - 挂载主文件系统
 * 
 * 指定一个文件系统成为主文件系统，系统通过它来访问其他文件系统
 */
PUBLIC int BOFS_MountMasterFS(struct BOFS_SuperBlock *superBlock)
{
    if (superBlock == NULL)
        return -1;

    /* 记录主超级块的指针，默认指定第一个 */
    if (masterSuperBlock == NULL) {
        masterSuperBlock = superBlock;
        printk("BOFS mount master at %x on device %d\n",
            masterSuperBlock, masterSuperBlock->deviceID);
        return 0;
    }
    return -1;
}

/**
 * BOFS_FormatFs - 格式化文件系统
 * @deiceID: 磁盘设备
 * @startSector: 起始扇区
 * @totalSectors: 扇区数
 * @metaIdCount: 元信息id数
 * 
 * 在一个磁盘上的某区域扇区中格式化一个文件系统，这样便于在磁盘分区后，
 * 在同一个磁盘上可以创建不同的文件系统或者是多个文件系统。
 * 格式化成功或者已经存在则返回超级块。
 */
PUBLIC struct BOFS_SuperBlock *BOFS_FormatFs(unsigned int deviceID, 
    unsigned int startSector,
    unsigned int totalSectors,
    unsigned int inodeNr)
{
    if (DeviceCheckID_OK(deviceID)) {
        printk(PART_ERROR "device %d not exist or not register!\n", deviceID);
        return NULL;
    }
    /* 扇区数不足 */
    if (totalSectors <= 0) {
        printk(PART_ERROR "no enough disk sectors!\n");
        return NULL;
    }
    
    printk(PART_TIP "fs info start sector %d sector count %d inode count %d\n",
        startSector, totalSectors, inodeNr);
    
    struct BOFS_SuperBlock *superBlock = BOFS_AllocSuperBlock();
    if (!superBlock)
        Panic("alloc super block failed for absurd fs!\n");
    
    /* 读取超级块，查看是否已经有文件系统，超级块位于分区开始处 */
    if (DeviceRead(deviceID, startSector, superBlock, 1)) {
        printk(PART_ERROR "device %d read failed!\n", deviceID);
    }
    
    /* 还没有格式化文件系统 */
    if (superBlock->magic != BOFS_SUPER_BLOCK_MAGIC) {
        /*no fs on disk, format it*/
        printk(PART_TIP "No fs on disk!\n");

        /* ----初始化超级块---- */
        
        superBlock->magic = BOFS_SUPER_BLOCK_MAGIC;
        superBlock->deviceID = deviceID;
    
        superBlock->superBlockLba = startSector;
        superBlock->totalSectors = totalSectors;	/*assume 10MB*/

        /* 位于超级块后面 */
        superBlock->sectorBitmapLba = superBlock->superBlockLba + 1;

        superBlock->sectorBitmapSectors = totalSectors / (8*SECTOR_SIZE);
		
        superBlock->inodeBitmapLba = superBlock->sectorBitmapLba + superBlock->sectorBitmapSectors;
        superBlock->inodeBitmapSectors = inodeNr/(8*SECTOR_SIZE);
        
        superBlock->inodeTableLba = superBlock->inodeBitmapLba + superBlock->inodeBitmapSectors;

        superBlock->inodeTableSectors = (inodeNr*sizeof(struct BOFS_Inode) )/(8*SECTOR_SIZE);
        
        superBlock->dataStartLba = superBlock->inodeTableLba + superBlock->inodeTableSectors;
        
        superBlock->rootInodeId = 0;
        superBlock->inodeNrInSector = SECTOR_SIZE/sizeof(struct BOFS_Inode);
        
        
        /* 
        设置扇区管理位图，inode管理位图，inode表，以及写入根目录
        */
        
        /*-----设置节点位图管理 -----*/
        char *iobuf = (char *)kmalloc(SECTOR_SIZE, GFP_KERNEL);
        if (iobuf == NULL) {
            printk(PART_ERROR "kmalloc for iobuf failed!\n");
            return NULL;
        }
        
        memset(iobuf, 0, SECTOR_SIZE);
        
        /* 使用1个inode，就是根目录的inode */
        iobuf[0] |= 1;
        
        if (DeviceWrite(deviceID, superBlock->inodeBitmapLba, iobuf, 1)) {
            printk(PART_ERROR "device write failed!\n");
            return NULL;
        }

        /*-----创建根节点-----*/
        struct BOFS_Inode rootInode;
        BOFS_CreateInode(&rootInode, 0, 
            (BOFS_IMODE_D|BOFS_IMODE_R|BOFS_IMODE_W),
            0, deviceID);
        
        /* 设置数据扇区以及节点数据大小 */
        rootInode.block[0] = superBlock->dataStartLba;
        rootInode.size = sizeof(struct BOFS_DirEntry);
        
        memset(iobuf, 0, SECTOR_SIZE);
        memcpy(iobuf, &rootInode, sizeof(struct BOFS_Inode));
        
        if (DeviceWrite(deviceID, superBlock->inodeTableLba, iobuf, 1)) {
            printk(PART_ERROR "device write failed!\n");
            return NULL;
        }

        //BOFS_DumpInode(&rootInode);
    
        /*-----创建根目录项-----*/
        
        struct BOFS_DirEntry rootDirEntry;

        BOFS_CreateDirEntry(&rootDirEntry, 0, BOFS_FILE_TYPE_DIRECTORY, 0, "/");

        memset(iobuf, 0, SECTOR_SIZE);
        memcpy(iobuf, &rootDirEntry, sizeof(struct BOFS_DirEntry));
        if (DeviceWrite(deviceID, superBlock->dataStartLba, iobuf, 1)) {
            printk(PART_ERROR "device write failed!\n");
            return NULL;
        }

        //BOFS_DumpDirEntry(&rootDirEntry);
        
        /* 扇区位图管理只管理数据区域的扇区，
        在这里，我们只使用了1个扇区来存放根目录的数据 */

        /*-----设置扇区管理位图 -----*/
        memset(iobuf, 0, SECTOR_SIZE);

        /* 第一个数据扇区被根目录占用 */
        iobuf[0] |= 1;
        if (DeviceWrite(deviceID, superBlock->sectorBitmapLba, iobuf, 1)) {
            printk(PART_ERROR "device write failed!\n");
            return NULL;
        }
        kfree(iobuf);

        /* 通过设定位图的大小来限制我们可以使用的空间 */
        superBlock->inodeBitmap.btmpBytesLen = inodeNr / 8;
        
        /* 计算管理区域使用了多少个扇区，用分区总扇区数减去这个值，
        就是数据区域的扇区数 */
        unsigned int usedSectors = superBlock->dataStartLba - \
            superBlock->sectorBitmapLba;

        unsigned int freeSectors = totalSectors - usedSectors;
        /* 大小都是以字节为单位进行保存 */
        superBlock->sectorBitmap.btmpBytesLen = freeSectors / 8;
        
        printk(PART_TIP "free sectors %d bitmap len %x\n", freeSectors,
            superBlock->sectorBitmap.btmpBytesLen);
        
        printk(PART_TIP "inode nr %d bitmap len %x\n", inodeNr,
            superBlock->inodeBitmap.btmpBytesLen);

        /* 写入超级块，保存所有修改的信息 */
        if (DeviceWrite(deviceID, startSector, superBlock, 1)) {
            printk(PART_ERROR "device write failed!\n");
            return NULL;
        }
       
    } else {
        printk(PART_TIP "Fs has existed.\n");
        /*printk("BookOS file system super block:\n");
        printk("device id:%d sectors:%d\nsector bitmap lba:%d sector bitmap sectors:%d\n", 
            superBlock->deviceID, superBlock->totalSectors, superBlock->sectorBitmapLba, 
            superBlock->sectorBitmapSectors);
        printk("inode bitmap lba:%d inode bitmap sectors:%d\ninode table lba:%d inode table sectors:%d data start lba:%d\n", 
            superBlock->inodeBitmapLba, superBlock->inodeBitmapSectors, 
            superBlock->inodeTableLba, superBlock->inodeTableSectors, 
            superBlock->dataStartLba);*/
    }

    BOFS_DumpSuperBlock(superBlock);

    if (BOFS_LoadBitmap(superBlock)) {
        printk("BOFS load bitmap failed!\n");
        return NULL;
    }
    
    if (BOFS_SuperBlockLoadBuffer(superBlock)) {
        printk("BOFS super block load buffer failed!\n");
        return NULL;
    }
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
    printk("sb size %d\n", sizeof(struct BOFS_SuperBlock));
    printk("dir size %d\n", sizeof(struct BOFS_Dir));
    printk("dir entry size %d\n", sizeof(struct BOFS_DirEntry));
    printk("inode size %d\n", sizeof(struct BOFS_Inode));
    
    //Spin("test");

    if (BOFS_OpenRootDir(superBlock)) {
        printk("BOFS open root dir failed!\n");
        return NULL;
    }

    /* 每次一个 */
   
    return superBlock;
}

/**
 * BOFS_MakeFS - 在分区上创建文件系统
 * @dpte: 分区表项
 * @deviceID: 文件系统所在的设备
 * 
 * 一个分区对应一个文件系统。
 */
PUBLIC struct BOFS_SuperBlock *BOFS_MakeFS(struct DiskPartitionTableEntry *dpte,
    unsigned int deviceID)
{
    struct BOFS_SuperBlock *sb = BOFS_FormatFs(deviceID, \
        dpte->startLBA, dpte->sectorsLimit, DEFAULT_MAX_INODE_NR);

    /* 格式化分区 */
    if (sb == NULL) {
        printk(PART_ERROR "BOFS format failed!\n");
        return NULL;
    }

    return sb;
}

PUBLIC void BOFS_Test()
{
    BOFS_DumpSuperBlock(masterSuperBlock);
    
    /* 创建挂载目录 */
    BOFS_MakeDir("/mnt");
    BOFS_MakeDir("/mnt/hda");

    BOFS_ListDir("/", 2);
    BOFS_ListDir("/mnt", 2);
    /* 
    BOFS_RemoveDir("/mnt/hda");
    BOFS_ListDir("/mnt", 2);
    
    BOFS_RemoveDir("/mnt");
    BOFS_ListDir("/", 2);
    */
    //BOFS_ListDir("/mnt", 0);
    
    //BOFS_RemoveDir("/mnt");

    /*BOFS_MakeDir("/hda0");
    */
    //BOFS_MakeDir("/hda1");

    //BOFS_MakeDir("/mnt/hda");
    
/*
    BOFS_MountDir("/hda1", "hda1");

    BOFS_MakeDir("/hda1/test5");
*/
    //BOFS_RemoveDir("/mnt/hda");

    BOFS_MountDir("/mnt/hda", "hda1");
    
    BOFS_MakeDir("/mnt/hda/test");
    
    BOFS_ListDir("/mnt/hda", 2);
    BOFS_ListDir("/mnt/hda/test", 0);
    BOFS_ListDir("/mnt", 0);
    //Spin("test");
    //BOFS_MakeDir("/mnt/hda/test2");
    
    BOFS_RemoveDir("/mnt/hda/test");
    
    BOFS_RemoveDir("/mnt/hda");
    
    /*BOFS_MakeDir("/mnt");
    
    BOFS_MakeDir("/bin");

    BOFS_MakeDir("/bin/vi");

    BOFS_ListDir("/bin", 0);
    
    BOFS_ListDir("/bin/vi", 1);
    BOFS_ListDir("/", 2);
    */
    /*struct BOFS_Dir *dir = BOFS_OpenDir("/bin");
    if (dir == NULL) {
        return;
    }

    struct BOFS_DirEntry *dirEntry;

    do {
        dirEntry = BOFS_ReadDir(dir);
        if (dirEntry == NULL) 
            break;

        BOFS_DumpDirEntry(dirEntry);
    } while (dirEntry != NULL);
    
    dir = BOFS_OpenDir("/bin/vi");
    if (dir == NULL) {
        return;
    }

    do {
        dirEntry = BOFS_ReadDir(dir);
        if (dirEntry == NULL) 
            break;

        BOFS_DumpDirEntry(dirEntry);
    } while (dirEntry != NULL);
    
     BOFS_RemoveDir("/");
    

    dir = BOFS_OpenDir("/");
    if (dir == NULL) {
        return;
    }
    
    do {
        dirEntry = BOFS_ReadDir(dir);
        if (dirEntry == NULL) 
            break;

        BOFS_DumpDirEntry(dirEntry);
    } while (dirEntry != NULL);
   
    BOFS_RewindDir(dir);
    do {
        dirEntry = BOFS_ReadDir(dir);
        if (dirEntry == NULL) 
            break;

        BOFS_DumpDirEntry(dirEntry);
    } while (dirEntry != NULL);*/
}


PUBLIC int BOFS_Init()
{
    BOFS_InitSuperBlockTable();
    masterSuperBlock = NULL;
    currentSuperBlock = NULL;
    
    return 0;
}