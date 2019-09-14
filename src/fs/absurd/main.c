/*
 * file:		fs/absurd/main.c
 * auther:		Jason Hu
 * time:		2019/9/1
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/slab.h>
#include <book/debug.h>
#include <share/string.h>
#include <book/deviceio.h>
#include <fs/absurd.h>
#include <share/string.h>
#include <driver/ide.h>

/* Absurd文件系统子系统 */
#include <fs/absurd/superblock.h>
#include <fs/absurd/meta.h>
#include <fs/absurd/direntry.h>

/*
主超级块和主根目录
用于记录第一个文件系统的超级块和根目录
*/
struct SuperBlock *mainSuperBlock;

struct DirEntry *mainRootDirEntry;

/**
 * AbsurdFormatFs - 格式化文件系统
 * @deiceID: 磁盘设备
 * @startSector: 起始扇区
 * @totalSectors: 扇区数
 * @metaIdCount: 元信息id数
 * 
 * 在一个磁盘上的某区域扇区中格式化一个文件系统，这样便于在磁盘分区后，
 * 在同一个磁盘上可以创建不同的文件系统或者是多个文件系统。
 */
PUBLIC int Absurd_FormatFs(unsigned int deviceID, 
    unsigned int startSector,
    unsigned int totalSectors,
    unsigned int metaIdCount)
{
    if (DeviceCheckID_OK(deviceID)) {
        printk(PART_ERROR "device %d not exist or not register!\n", deviceID);
        return -1;
    }
    /* 扇区数不足 */
    if (totalSectors <= 0) {
        printk(PART_ERROR "no enough disk sectors!\n");
        return -1;
    }
    /*
    printk(PART_TIP "fs info start sector %d sector count %d meta id count %d\n",
        startSector, totalSectors, metaIdCount);
    */

    struct SuperBlock *superBlock = AllocSuperBlock();
    if (!superBlock)
        Panic("alloc super block failed for absurd fs!\n");
    
    /* 读取超级块，查看是否已经有文件系统，超级块位于分区开始处 */
    if (DeviceRead(deviceID, startSector, superBlock, 1)) {
        printk(PART_ERROR "device write failed!\n");
    }
    
    /* 记录主超级块的指针 */
    if (mainSuperBlock == NULL) {
        mainSuperBlock = superBlock;
    }


    /* 还没有格式化文件系统 */
    if (superBlock->magic != ABSURD_SUPER_BLOCK_MAGIC) {
        /* ----初始化超级块---- */

        MetaInfoInit(&superBlock->meta, "superblock for absurd",
            startSector, META_SUPERBLOCK, deviceID);

        //printk(PART_TIP "meta info size %d\n", SIZEOF_META_INFO);

        /* 把0（引导扇区）扇区和1（超级块所在）扇区预留 */

        SetMetaData(&superBlock->meta, superBlock->meta.selfLocation + 1, totalSectors - 2);

        /* 设置元信息区域数据大小 */
        superBlock->meta.size = superBlock->meta.data.sectors * SECTOR_SIZE;

        ShowMetaInfo(&superBlock->meta);
        
        /* ----初始化扇区管理位图---- */
        
        /* 记录超级块扇区管理位图信息 */
        superBlock->sectorBitmapLba = superBlock->meta.data.location;
        /* 计算存放扇区需要多少扇区, 8是一个字节中有8个位 */
        superBlock->sectorBitmapSectors = superBlock->meta.data.sectors / (8 * SECTOR_SIZE);
        
        /* 设置位图字节长度 */
        superBlock->sectorBitmap.btmpBytesLen = superBlock->meta.data.sectors / 8;
        
        /* ----初始化元信息管理位图---- */

        /* 放到扇区位图后面 */
        superBlock->metaIdBitmapLba = superBlock->sectorBitmapLba + 
            superBlock->sectorBitmapSectors;
        superBlock->metaIdBitmapSectors = metaIdCount / (8 * SECTOR_SIZE);
        
        /* 设置位图字节长度 */
        superBlock->metaIdBitmap.btmpBytesLen = metaIdCount / 8;

        /* ----设置数据区域扇区信息---- */
        superBlock->dataAreaLba = superBlock->metaIdBitmapLba + 
            superBlock->metaIdBitmapSectors;

        /* 计算已经使用了多少个扇区，然后修改扇区位图 */
        unsigned int usedSectors = superBlock->sectorBitmapSectors + 
            superBlock->metaIdBitmapSectors;

        superBlock->dataAreaSectors = superBlock->meta.data.sectors - usedSectors;

        printk("sector bitmap lba %d sector bitmap sectors %d\n", 
            superBlock->sectorBitmapLba, superBlock->sectorBitmapSectors);
        
        printk("meta id bitmap lba %d meta id bitmap sectors %d\n", 
            superBlock->metaIdBitmapLba, superBlock->metaIdBitmapSectors);

        printk("data area lba %d data area sectors %d\n", 
            superBlock->dataAreaLba, superBlock->dataAreaSectors);
        
        /* 魔数标志 */
        superBlock->magic = ABSURD_SUPER_BLOCK_MAGIC;


        /* 把超级块写入磁盘 */
        if (DeviceWrite(deviceID, superBlock->meta.selfLocation, superBlock, 1)) {
            printk(PART_ERROR "device write failed!\n");
        }
        
        /* ----初始化根目录---- */

        /* 创建根目录 */

        /* 分配一个扇区来储存根目录 */
        superBlock->rootDirEntry = kmalloc(SECTOR_SIZE, GFP_KERNEL);
        struct DirEntry *rootDirEntry = superBlock->rootDirEntry;
        
        /* 根目录位于超级块的第一个扇区 */
        MetaInfoInit(&rootDirEntry->meta, "/",
            superBlock->dataAreaLba, META_DIRECTORY, deviceID);
        
        /* 根目录的数据区域是除开根目录外的所有扇区数据 */
        SetMetaData(&rootDirEntry->meta, superBlock->dataAreaLba + 1,
            superBlock->dataAreaSectors -1);

        /* 由于现在根目录什么也没有，所以它的大小是0 */
        rootDirEntry->meta.size = 0;
        
        /* 分配一个meta id，这里使用0 id，因为超级块的id不算 */
        rootDirEntry->meta.id = 0;
        
        /* 设定设备id */
        rootDirEntry->meta.deviceID = deviceID;

        /* 分配一个管理扇区，这里就增加一个扇区的使用就可以了 */
        usedSectors++;
        
        /* 现在根目录数据就已经写好了，可以写入到磁盘中去 */
        if (DeviceWrite(deviceID, rootDirEntry->meta.selfLocation,
            rootDirEntry, 1)) {
            printk("write root dir entry failed!\n");

            return -1;
        }
        
        /* 打印根目录信息 */
        ShowMetaInfo(&rootDirEntry->meta);

        /* ----修改扇区管理位图---- */

        /* 使用扇区数所占用的字节数 */
        unsigned int usedSectorsBytes = usedSectors / 8;
        
        /* 使用扇区数还剩余下的位数 */
        unsigned int usedSectorsLeftBits = usedSectors % 8;
        
        /* 如果剩余字节位还有，就增加一个占用字节 */
        if (usedSectorsLeftBits)
            usedSectorsBytes++;
        
        /* 计算占用的扇区数 */
        unsigned int sectorBitmapUsedSectros = usedSectorsBytes / SECTOR_SIZE;
        
        /* 计算占用扇区部分的字节数 */
        unsigned int sectorBitmapLeftBytes = usedSectorsBytes % SECTOR_SIZE;
        /*
        printk("used sectors %d used bytes %x left bytes %x\n", usedSectors, 
            usedSectorsBytes, usedSectorsLeftBits);

        printk("used bitmap sectors %d left bitmap bytes %x\n", 
            sectorBitmapUsedSectros, sectorBitmapLeftBytes);
        */
        /* 把已经使用的扇区标识使用（扇区管理和元信息管理占用的空间） */
        
        /* 把位图写入磁盘 */
        char *iobuf = kmalloc(SECTOR_SIZE, GFP_KERNEL);
        if (iobuf == NULL)
            Panic("kmalloc for iobuf failed!\n");

        /* 把整个扇区都标识已经使用 */
        memset(iobuf, 0xff, SECTOR_SIZE);
        int i;
        for (i = 0; i < sectorBitmapUsedSectros; i++) {
            //printk("write a full sector at %d\n", superBlock->sectorBitmapLba + i);
            /* 每个扇区都要写入 */
            if (DeviceWrite(deviceID, superBlock->sectorBitmapLba + i, 
                iobuf, 1)) {
                printk(PART_ERROR "device write sector bitmap failed!\n");
                return -1;
            }
        }

        /* 清空buffer */
        memset(iobuf, 0, SECTOR_SIZE);
        
        /* 还有剩下的字节 */
        if (sectorBitmapLeftBytes) {       
            //printk("have left %d bytes\n", sectorBitmapLeftBytes);
            
            for (i = 0; i < sectorBitmapLeftBytes; i++) {
                memset(&iobuf[i], 0xff, 1);
                /* 到达最后一个字节 */
                if (i == sectorBitmapLeftBytes - 1) {
                    /* 只保留余下的位那么多位 */

                    /* 先置0 */
                    iobuf[i] = 0;

                    int j;
                    /* 再对每1位进行判断 */
                    for (j = 0; j < usedSectorsLeftBits; j++) {
                        /* 把该位置1 */
                        iobuf[i] |= 1 << j;
                    }
                    //printk("data:%x\n", iobuf[i]);
            
                } 
            }
            //printk("write a partial sector at %d\n", superBlock->sectorBitmapLba + sectorBitmapUsedSectros);
            
            /* 缓冲区数据写好后，就准备写入磁盘 */
            if (DeviceWrite(deviceID, superBlock->sectorBitmapLba + sectorBitmapUsedSectros, 
                iobuf, 1)) {
                printk(PART_ERROR "device write sector bitmap failed!\n");
                return -1;
            }
        }
        /* 现在已经把扇区位图管理和元信息位图已使用的扇区标记成已经使用 */
        
        /* ----修改元信息ID管理位图---- */
        memset(iobuf, 0, SECTOR_SIZE);

        /* 由于只使用入了1个id(根目录ID)，所以只设定一个位 */
        iobuf[0] |= 1;

        /* 缓冲区数据写好后，就准备写入磁盘 */
        if (DeviceWrite(deviceID, superBlock->metaIdBitmapLba, iobuf, 1)) {
            printk(PART_ERROR "device write meta id bitmap failed!\n");
            return -1;
        }
    } else {
        /* 已经格式化文件系统 */

        /* 打开根目录 */
        /* 分配一个扇区来储存根目录 */
        superBlock->rootDirEntry = kmalloc(SECTOR_SIZE, GFP_KERNEL);
        struct DirEntry *rootDirEntry = superBlock->rootDirEntry;
        
        /* 直接从磁盘读取根目录 */
        if (DeviceRead(deviceID, superBlock->dataAreaLba, rootDirEntry, 1)) {
            printk(PART_ERROR "device read root dir entry failed!\n");
            return -1;
        }

        /* 打印根目录信息 */
        ShowMetaInfo(&rootDirEntry->meta);

    }

    /* 记录主超级块的指针 */
    if (mainRootDirEntry == NULL) {
        mainRootDirEntry = superBlock->rootDirEntry;
    }

    /* 加载sectors位图管理 */

    if (LoadSectorBitmap(superBlock)) {
        return -1;
    }
    
    /* 加载meta id位图管理 */
    if (LoadMetaIdBitmap(superBlock)) {
        return -1;
    }

    return 0;
}

/**
 * InstallAbsurdFS - 在分区上安装文件系统
 * @dpte: 分区表项
 * @deviceID: 文件系统所在的设备
 * 
 * 一个分区对应一个文件系统。
 */
PUBLIC int InstallAbsurdFS(struct DiskPartitionTableEntry *dpte,
    unsigned int deviceID)
{
    /* 格式化分区 */
    if (Absurd_FormatFs(deviceID, dpte->startLBA, dpte->sectorsLimit,
        DEFAULT_META_ID_NR)) {
        
        printk(PART_ERROR "format absurd fs failed!\n");
        return -1;
    }
    
    ShowMetaInfo(&mainRootDirEntry->meta);



    
    return 0;
}

/**
 * InitAbsurdFS - 初始化怪诞文件系统
 * 
 */
PUBLIC int InitAbsurdFS()
{
    PART_START("Absurd FS");

    /* 初始化超级块环境 */
    if (InitSuperBlockTable()) {
        printk(PART_ERROR "init super block table failed!\n");
        return -1;
    }
    
    mainSuperBlock = NULL;
    mainRootDirEntry = NULL;

    PART_END();

    return 0;
}
