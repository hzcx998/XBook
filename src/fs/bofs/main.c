/*
 * file:		fs/bofs/main.c
 * auther:		Jason Hu
 * time:		2019/9/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/string.h>
#include <driver/ide.h>
#include <share/vsprintf.h>
#include <share/const.h>

#include <fs/partition.h>
#include <fs/bofs/super_block.h>
#include <fs/bofs/inode.h>
#include <fs/bofs/dir_entry.h>
#include <fs/bofs/bitmap.h>
#include <fs/bofs/dir.h>
#include <fs/bofs/file.h>
#include <fs/bofs/device.h>

EXTERN struct List AllDeviceListHead;

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
    /*if (DeviceCheckID_OK(deviceID)) {
        printk(PART_ERROR "device %d not exist or not register!\n", deviceID);
        return NULL;
    }*/
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

    //BOFS_DumpSuperBlock(superBlock);

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
    /*printk("sb size %d\n", sizeof(struct BOFS_SuperBlock));
    printk("dir size %d\n", sizeof(struct BOFS_Dir));
    printk("dir entry size %d\n", sizeof(struct BOFS_DirEntry));
    printk("inode size %d\n", sizeof(struct BOFS_Inode));
    */
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

PUBLIC void BOFS_Setup()
{
    //BOFS_DumpSuperBlock(masterSuperBlock);
    //Spin("1");
    
    if (BOFS_Access("/dev", BOFS_F_OK)) {
        BOFS_MakeDir("/dev");
    }
    /* 创建设备文件，再往设备文件下面添加设备之前，先把里面得所有设备都删除 */
    
    /* 扫描文件描述信息，并创建对应得设备文件 */
    /*if (BOFS_MakeDevice("/dev/hda", BOFS_FILE_TYPE_BLOCK, DEVICE_HDD0)) {
        printk("make device failed!\n");
    }
    
    if (BOFS_MakeDevice("/dev/hda0", BOFS_FILE_TYPE_BLOCK, DEVICE_HDD0)) {
        printk("make device failed!\n");
    }

    if (BOFS_MakeDevice("/dev/hda1", BOFS_FILE_TYPE_BLOCK, DEVICE_HDD0)) {
        printk("make device failed!\n");
    }

    if (BOFS_MakeDevice("/dev/hdb0", BOFS_FILE_TYPE_BLOCK, DEVICE_HDD1)) {
        printk("make device failed!\n");
    }
    
    if (BOFS_MakeDevice("/dev/hdc0", BOFS_FILE_TYPE_BLOCK, DEVICE_HDD2)) {
        printk("make device failed!\n");
    }*/

    /* 根据已经有的设备创建设备文件 */

    char devpath[64];
    struct Device *device;
    char fsDeviceType;
    ListForEachOwner(device, &AllDeviceListHead, list) {
        memset(devpath, 0, 64);
    
        // 生成路径
        strcat(devpath, "/dev/");
        strcat(devpath, device->name);
        
        /* 选择类型 */
        switch (device->type)
        {
        case DEV_TYPE_BLOCK:
            fsDeviceType = BOFS_FILE_TYPE_BLOCK;
            break;
        case DEV_TYPE_CHAR:
            fsDeviceType = BOFS_FILE_TYPE_CHAR;
            break;
        case DEV_TYPE_NET:
            fsDeviceType = BOFS_FILE_TYPE_NET;
            break;
        default:
            Panic("make device under /dev failed!\n");
            break;
        }

        //printk("device path: %s type %x device id %d\n", devpath, fsDeviceType, device->deviceID);
        /* 创建设备文件 */
        if (BOFS_MakeDevice(devpath, fsDeviceType, device->devno)) {
            printk("make device failed!\n");
        }
        //BOFS_DumpSuperBlock(device->pointer);
    }


    /*
    int fd = BOFS_Open("/dev/hda", BOFS_O_RDWR);
    if (fd < 0)
        printk("fd error!\n");
    BOFS_DumpFD(fd);

    
    char buf[512];
    printk("fd:%d\n", fd);
    BOFS_Lseek(fd, 2, BOFS_SEEK_SET);

    if (512 != BOFS_Read(fd, buf, 512)) {
        printk("read error!\n");
    }
    int i;
    for (i = 0; i < 16; i++) {
        printk("buf %x",buf[i]);
    
    }
    
    memset(buf, 0xff, 512);

    BOFS_Lseek(fd, 2, BOFS_SEEK_SET);
    
    if (512 != BOFS_Write(fd, buf, 512)) {
        printk("write error!\n");
    }

    BOFS_Lseek(fd, 2, BOFS_SEEK_SET);
    memset(buf, 0, 512);
    
    if (512 != BOFS_Read(fd, buf, 512)) {
        printk("read error!\n");
    }
    
    for (i = 0; i < 16; i++) {
        printk("buf %x",buf[i]);
    }
*/
 
    /* 创建挂载目录 */
    
    if (BOFS_Access("/mnt", BOFS_F_OK)) {
        BOFS_MakeDir("/mnt");
    }
    
    /* 创建根目录 */

    if (BOFS_Access("/root", BOFS_F_OK)) {
        BOFS_MakeDir("/root");
    }
    /* 挂载根目录，这样就不能直接对设备卸载了 */
    BOFS_MountDir("/dev/hda0", "/root");

    /* 创建用户目录 */

    if (BOFS_Access("/user", BOFS_F_OK)) {
        BOFS_MakeDir("/user");
    }

    /* 创建程序目录 */

    if (BOFS_Access("/bin", BOFS_F_OK)) {
        BOFS_MakeDir("/bin");
    }
    /*
    BOFS_UnmountDir("/root");

    BOFS_Unlink("/dev/hda0");*/

    //BOFS_MakeDir("/root/new");

    /*BOFS_ListDir("/root", 0);
    BOFS_ListDir("/root/new", 0);
    
    BOFS_ListDir("/", 0);*/

    /*
    int sectors = 0;
    BOFS_Ioctl(fd, ATA_IO_SECTORS, (int)&sectors);
    
    printk("get sectors from disk:%d\n", sectors);
    BOFS_Close(fd);
    */
    //Spin("1");

    /* 要挂载的目录 */
    /*BOFS_MakeDir("/mnt/winc");
    BOFS_MountDir("/dev/hda1", "/mnt/winc");
    
    BOFS_MakeDir("/mnt/winc/test");
    BOFS_ListDir("/mnt/winc", 0);
    
    BOFS_MakeDir("/mnt/wind");
    BOFS_MountDir("/dev/hdb0", "/mnt/wind");
    
    BOFS_MakeDir("/mnt/wind/test");
    BOFS_ListDir("/mnt/wind", 0);
    
    BOFS_MakeDir("/mnt/wine");
    BOFS_MountDir("/dev/hdc0", "/mnt/wine");
    
    BOFS_MakeDir("/mnt/wine/test");
    BOFS_ListDir("/mnt/wine", 0);
*/
    //BOFS_ListDir("/dev", 0);

    /*
    BOFS_MakeDir("/mnt/wine");
    BOFS_MountDir("/mnt/wine", "hdc0");
    
    BOFS_MakeDir("/mnt/winf");
    BOFS_MountDir("/mnt/winf", "hdd0");*/
    //Spin("1");
    /*
    BOFS_ListDir("/", 2);
    BOFS_ListDir("/mnt", 2);*/
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

    //BOFS_MakeDir("/mnt/winc/test");
     //Spin("test");
    
    //BOFS_ListDir("/mnt/winc", 2);
    //Spin("test");
    /*
    BOFS_ListDir("/mnt/hda/test", 0);
    BOFS_ListDir("/mnt", 0);*/
    //BOFS_MakeDir("/mnt/hda/test2");
    
    //BOFS_RemoveDir("/mnt/hda/test");
    
    //BOFS_RemoveDir("/mnt/hda");
    
    //BOFS_MountDir("/mnt/hda/test", "hda1");
    /*BOFS_UnmountDir("/mnt/hda");
    BOFS_RemoveDir("/mnt/hda/test");
    BOFS_ListDir("/mnt/hda", 2);*/
    
    /*BOFS_MakeDir("/mnt");
    
    BOFS_MakeDir("/bin");

    BOFS_MakeDir("/bin/vi");

    BOFS_ListDir("/bin", 0);
    
    BOFS_ListDir("/bin/vi", 1);
    BOFS_ListDir("/", 2);
    */
}

PRIVATE void BOFS_Test()
{

    return;
    //Spin("test");


    int fd = BOFS_Open("/test2", BOFS_O_CREAT | BOFS_O_RDWR);
    if (fd < 0) {
        printk("file open failed!\n");
    }

    //BOFS_DumpFD(fd);
    
    BOFS_Close(fd);    

    fd = BOFS_Open("/test2", BOFS_O_CREAT | BOFS_O_RDWR);
    if (fd < 0) {
        printk("file open failed!\n");
    }

    //BOFS_DumpFD(fd);

    BOFS_Close(fd);    

    fd = BOFS_Open("/mnt/winc/open", BOFS_O_CREAT | BOFS_O_RDWR);
    if (fd < 0) {
        printk("file open failed!\n");
    }

    BOFS_ListDir("/mnt/winc", 1);
    //BOFS_DumpFD(fd);

    BOFS_Close(fd);    
    
    fd = BOFS_Open("/mnt/wind/open", BOFS_O_CREAT | BOFS_O_RDWR);
    if (fd < 0) {
        printk("file open failed!\n");
    }
    BOFS_ListDir("/mnt/wind", 1);

    fd = BOFS_Open("/mnt/wine/open", BOFS_O_CREAT | BOFS_O_RDWR);
    if (fd < 0) {
        printk("file open failed!\n");
    }
    BOFS_ListDir("/mnt/wine", 1);
    fd = BOFS_Open("/mnt/winf/open", BOFS_O_CREAT | BOFS_O_RDWR);
    if (fd < 0) {
        printk("file open failed!\n");
    }
    BOFS_ListDir("/mnt/winf", 1);

    BOFS_ListDir("/mnt", 2);
    

    //BOFS_DumpFD(fd);
/*
    if(!BOFS_Access("/mnt/hda/open", BOFS_F_OK)) {
        printk("file exist!\n");
    } else {
        printk("file not exist!\n");
    }*/
/*
    if(!BOFS_Access("/mnt/hda/open2", BOFS_F_OK)) {
        printk("file exist!\n");
    } else {
        printk("file not exist!\n");
    }
    
    if(!BOFS_Access("/mnt/hda/open", BOFS_R_OK | BOFS_W_OK)) {
        printk("file RW!\n");
    } else {
        printk("file not RW!\n");
    }*/
/*
    int mode = BOFS_GetMode("/mnt/hda/open");
    printk("get mode:%x\n", mode);

    BOFS_SetMode("/mnt/hda/open", mode | BOFS_IMODE_X);

    mode = BOFS_GetMode("/mnt/hda/open");
    printk("set mode:%x\n", mode);
    */
    /*struct BOFS_Stat stat;
    BOFS_Stat("/", &stat);

    printk("stat: size %d mode %x type %d device %d\n", stat.size, stat.mode, stat.type, stat.device);
    
    BOFS_Stat("/mnt", &stat);

    printk("stat: size %d mode %x type %d device %d\n", stat.size, stat.mode, stat.type, stat.device);
    */
    /*char cwd[32];
    BOFS_GetCWD(cwd, 32);
    printk(cwd);

    BOFS_ChangeCWD("/mnt");
    BOFS_GetCWD(cwd, 32);
    printk(cwd);
    BOFS_ListDir("/mnt/winc", 2);
    
    BOFS_ResetName("/mnt/winc/open", "rename");

    BOFS_ListDir("/mnt/winc", 2);*/

    /*
    fd = BOFS_Open("/mnt/hda", BOFS_O_CREAT | BOFS_O_RDWR);
    if (fd < 0) {
        printk("file open failed!\n");
    }
    
    BOFS_DumpFD(fd);
    */
    //BOFS_Close(fd);   
    /*char *buf = kmalloc(1024*100, GFP_KERNEL); 
    if (buf == NULL)
    {
        Panic("alloc failed!");
    }
    
    memset(buf, 0x5a, 1024*100);
    BOFS_Write(fd, buf, 1024*100);

    BOFS_Lseek(fd, 0, BOFS_SEEK_SET);
    memset(buf, 0, 1024*100);
    if (BOFS_Read(fd, buf, 1024*100) != 1024*100) {
        printk("read error!\n");
    }
    int i;
    for (i = 0; i < 32; i++) {
        printk("%x", buf[i]);
    }*/
/*
    BOFS_Unlink("/mnt/hda/open");
    BOFS_Unlink("/mnt/hda");
    BOFS_Unlink("/mnt");
    //BOFS_Unlink("/test2");
    BOFS_Unlink("/");
    */
}

PUBLIC int BOFS_Init()
{
    /* 初始化文件系统环境 */
    BOFS_InitSuperBlockTable();
    masterSuperBlock = NULL;
    currentSuperBlock = NULL;
    
    /* 创建文件系统 */
	unsigned int deviceID = DEV_HDA;
    int i;

	for (i = 0; i < ideDiskFound; i++) {
		/* 打印分区信息 */
		/*printk("DPTE: flags %x type %x start lba %d limit sectors %d\n", 
			dptTable[0]->DPTE[0].flags, dptTable[0]->DPTE[0].type,
			dptTable[0]->DPTE[0].startLBA, dptTable[0]->DPTE[0].sectorsLimit);
		*/
		/* 现在已经拥有分区信息，可以在分区上创建文件系统了 */
		
		/* 在分区上创建文件系统 */

		
        char targe[4] = {'a', 'b', 'c', 'd'};

		//struct Device *device;
		char name[DEVICE_NAME_LEN];
		
        /* ----记录设备---- */

        sprintf(name, "hd%c", targe[i]);
        /* 如果不是分区设备，就把pointer设置成NULL，这样就不能挂载 */
        /*device = CreateDevice(name, NULL, DEVICE_TYPE_BLOCK, deviceID);
		if (device == NULL)
			Panic("create device %s failed!\n", name);*/
        /* 添加到设备信息记录管理器  */
        //AddDevice(device);
        /* ----记录设备分区---- */

        /* 创建块设备，并把块设备添加到块设备系统中 */
        /* 创建并挂载成一个主文件系统 */
		/*struct BOFS_SuperBlock *sb = BOFS_MakeFS(&dptTable[i]->DPTE[0], deviceID);
		
		if (BOFS_MountMasterFS(sb) && i == 0) {
			Panic("BOFS mount master failed!\n");
		}

        sprintf(name, "hd%c%d", targe[i], 0);
		device = CreateDevice(name, sb, DEVICE_TYPE_BLOCK, deviceID);
		if (device == NULL)
			Panic("create device hda0 failed!\n");
		AddDevice(device);
		
		sb = BOFS_MakeFS(&dptTable[i]->DPTE[1], deviceID);
		if (sb == NULL) {
			printk("make fs failed!\n");
			return -1;
		}
        sprintf(name, "hd%c%d", targe[i], 1);
		device = CreateDevice(name, sb, DEVICE_TYPE_BLOCK, deviceID);

		if (device == NULL)
			Panic("create device hda0 failed!\n");
		AddDevice(device);
*/
		/* 指向下一个设备 */
		deviceID++;
	}

    //DumpDevices();

    //Spin("test");
    /* 做一些重要的步骤 */
    BOFS_Setup();

    /* 做测试 */
    BOFS_Test();

    return 0;
}


