/*
 * file:		fs/vfs.c
 * auther:		Jason Hu
 * time:		2019/8/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/math.h>
#include <book/vmarea.h>
#include <fs/partition.h>
#include <fs/bofs/bofs.h>
#include <fs/bofs/file.h>
#include <fs/interface.h>
#include <book/blk-dev.h>

PUBLIC int SysOpen(const char *path, unsigned int flags)
{
	return BOFS_Open(path, flags);
}

PUBLIC int SysLseek(int fd, unsigned int offset, char flags)
{
	return BOFS_Lseek(fd, offset, flags);
}

PUBLIC int SysRead(int fd, void *buffer, unsigned int size)
{
	return BOFS_Read(fd, buffer, size);
}

PUBLIC int SysWrite(int fd, void *buffer, unsigned int size)
{
	return BOFS_Write(fd, buffer, size);
}

PUBLIC int SysClose(int fd)
{
	return BOFS_Close(fd);
}

PUBLIC int SysStat(const char *path, void *buf)
{	
	return BOFS_Stat(path, (struct BOFS_Stat *)buf);
}

PUBLIC int SysUnlink(const char *pathname)
{
	return BOFS_Unlink(pathname);
}

PUBLIC int SysIoctl(int fd, int cmd, int arg)
{
	return BOFS_Ioctl(fd, cmd, arg);
}

PUBLIC int SysAccess(const char *pathname, int mode)
{
	return BOFS_Access(pathname, mode);
}

PUBLIC int SysGetMode(const char* pathname)
{
	return BOFS_GetMode(pathname);
}

PUBLIC int SysSetMode(const char* pathname, int mode)
{
	return BOFS_SetMode(pathname, mode);
}

PUBLIC int SysMakeDir(const char *pathname)
{
	return BOFS_MakeDir(pathname);
}

PUBLIC int SysRemoveDir(const char *pathname)
{
	return BOFS_RemoveDir(pathname);
}

PUBLIC int SysMountDir(const char *devpath, const char *dirpath)
{
	return BOFS_MountDir(devpath, dirpath);
}

PUBLIC int SysUnmountDir(const char *dirpath)
{
	return BOFS_UnmountDir(dirpath);
}

PUBLIC int SysGetCWD(char* buf, unsigned int size)
{
	return BOFS_GetCWD(buf, size);
}

PUBLIC int SysChangeCWD(const char *pathname)
{
	return BOFS_ChangeCWD(pathname);
}

PUBLIC int SysChangeName(const char *pathname, char *name)
{
	return BOFS_ResetName(pathname, name);
}

PUBLIC DIR *SysOpenDir(const char *pathname)
{
	struct BOFS_Dir *dir = BOFS_OpenDir(pathname);
	if (dir == NULL)
		return NULL;

	return (DIR *)dir;
}

PUBLIC void SysCloseDir(DIR *dir)
{
	BOFS_CloseDir((struct BOFS_Dir *)dir);
}

PUBLIC int SysReadDir(DIR *dir, DIRENT *buf)
{
	struct BOFS_DirEntry *dirEntry = BOFS_ReadDir((struct BOFS_Dir *)dir);
	if (dirEntry == NULL)
		return -1;

	/* 复制目录项内容 */
	buf->inode = dirEntry->inode;
	buf->type = dirEntry->type;
	memset(buf->name, 0, NAME_MAX);
	strcpy(buf->name, dirEntry->name);

	/* 获取节点信息，并填充到缓冲区中 */
	struct BOFS_Inode inode;
	
	struct BOFS_Dir *bdir = (struct BOFS_Dir *)dir;

	BOFS_LoadInodeByID(&inode, dirEntry->inode, bdir->superBlock);
	
	buf->deviceID = inode.deviceID;
	buf->otherDeviceID = inode.otherDeviceID;
	buf->mode = inode.mode;
	buf->links = inode.links;
	buf->size = inode.size;
	buf->crttime = inode.crttime;
	buf->mdftime = inode.mdftime;
	buf->acstime = inode.acstime;

	return 0;
}

PUBLIC void SysRewindDir(DIR *dir)
{
	BOFS_RewindDir((struct BOFS_Dir *)dir);
}

/* 同步磁盘上的数据到文件系统 */
#define SYNC_DISK_DATA

/* 数据是在软盘上的 */
#define DATA_ON_FLOPPY
#define FLOPPY_DATA_ADDR	0x80042000


/* 数据是在软盘上的 */
#define DATA_ON_IDE

/* 要写入文件系统的文件 */
#define FILE_ID 1

#if FILE_ID == 1
	#define FILE_NAME "/bin/init"
	#define FILE_SECTORS 30
#elif FILE_ID == 2
	#define FILE_NAME "/bin/shell"
	#define FILE_SECTORS 30
#elif FILE_ID == 3
	#define FILE_NAME "/bin/test"
	#define FILE_SECTORS 100
#endif

#define FILE_SIZE (FILE_SECTORS * SECTOR_SIZE)

PRIVATE void WriteDataToFS()
{
	#ifdef SYNC_DISK_DATA
	char *buf = kmalloc(FILE_SECTORS * SECTOR_SIZE, GFP_KERNEL);
	if (buf == NULL) {
		Panic("kmalloc for buf failed!\n");
	}
	
	/* 把文件加载到文件系统中来 */
	int fd = SysOpen(FILE_NAME, O_CREAT | O_RDWR);
    if (fd < 0) {
        printk("file open failed!\n");
		return;
    }
	
	memset(buf, 0, FILE_SECTORS * SECTOR_SIZE);
	
	/* 根据数据存在的不同形式选择不同的加载方式 */
	#ifdef DATA_ON_FLOPPY
		memcpy(buf, (void *)FLOPPY_DATA_ADDR, FILE_SIZE);
	#endif

	#ifdef DATA_ON_IDE


	#endif
	
	if (SysWrite(fd, buf, FILE_SIZE) != FILE_SIZE) {
		printk("write failed!\n");
	}

	SysLseek(fd, 0, SEEK_SET);
	if (SysRead(fd, buf, FILE_SIZE) != FILE_SIZE) {
		printk("read failed!\n");
	}



	kfree(buf);
	SysClose(fd);
	printk("load file sucess!\n");
	//Panic("test");
	#endif
}


#include <book/blk-disk.h>

/**
 * 1.获取磁盘以及对应得分区
 * 2.在分区上面创建一个文件系统
 * 3.把文件系统挂载到内存中
 */

/**  
 * BOFS_FormatOnPart - 在分区上面创建一个文件系统
 * @part: 要操作分区
 * @inodeNr: 节点数
 * 
 * 格式化完成后返回超级块
 */
struct BOFS_SuperBlock *BOFS_FormatOnPart(struct DiskPartition *part, unsigned int inodeNr)
{
	/* 扇区数不足 */
    if (part->sectorCounts <= 0) {
        printk(PART_ERROR "no enough disk sectors!\n");
        return NULL;
    }
    
    printk(PART_TIP "fs info start sector %d sector count %d inode count %d\n",
        part->startSector, part->sectorCounts, inodeNr);
    
	unsigned int devno = MKDEV(part->disk->major, part->disk->firstMinor); 
	printk("devno: %x\n", devno);

	part->sb = kmalloc(SECTOR_SIZE, GFP_KERNEL);
	if (!part->sb)
        Panic("alloc super block failed for fs!\n");
    
    struct BOFS_SuperBlock *superBlock = part->sb;
    
    /* 读取超级块，查看是否已经有文件系统，超级块位于分区开始处 */
    if (!BlockRead(devno, part->startSector + 1, superBlock)) {
        printk(PART_ERROR "device %d read failed!\n", devno);
    }
    
    /* 还没有格式化文件系统 */
    if (superBlock->magic != BOFS_SUPER_BLOCK_MAGIC) {
        /*no fs on disk, format it*/
        printk(PART_TIP "No fs on disk!\n");

        /* ----初始化超级块---- */
        
        superBlock->magic = BOFS_SUPER_BLOCK_MAGIC;
        superBlock->deviceID = devno;

		superBlock->totalSectors = part->sectorCounts;

		/* 跳过引导扇区 */
        superBlock->superBlockLba = part->startSector + 1;

        /* 位于超级块后面 */
        superBlock->sectorBitmapLba = superBlock->superBlockLba + 1;

        superBlock->sectorBitmapSectors = part->sectorCounts / (8*SECTOR_SIZE);
		
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
        
        if (!BlockWrite(devno, superBlock->inodeBitmapLba, iobuf, 1)) {
            printk(PART_ERROR "device write failed!\n");
            return NULL;
        }

        /*-----创建根节点-----*/
        struct BOFS_Inode rootInode;
        BOFS_CreateInode(&rootInode, 0, 
            (BOFS_IMODE_D|BOFS_IMODE_R|BOFS_IMODE_W),
            0, devno);
        
        /* 设置数据扇区以及节点数据大小 */
        rootInode.block[0] = superBlock->dataStartLba;
        rootInode.size = sizeof(struct BOFS_DirEntry);
        
        memset(iobuf, 0, SECTOR_SIZE);
        memcpy(iobuf, &rootInode, sizeof(struct BOFS_Inode));
        
        if (!BlockWrite(devno, superBlock->inodeTableLba, iobuf, 1)) {
            printk(PART_ERROR "device write failed!\n");
            return NULL;
        }

        //BOFS_DumpInode(&rootInode);
    
        /*-----创建根目录项-----*/
        
        struct BOFS_DirEntry rootDirEntry;

        BOFS_CreateDirEntry(&rootDirEntry, 0, BOFS_FILE_TYPE_DIRECTORY, 0, "/");

        memset(iobuf, 0, SECTOR_SIZE);
        memcpy(iobuf, &rootDirEntry, sizeof(struct BOFS_DirEntry));
        if (!BlockWrite(devno, superBlock->dataStartLba, iobuf, 1)) {
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
        if (!BlockWrite(devno, superBlock->sectorBitmapLba, iobuf, 1)) {
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

        unsigned int freeSectors = part->sectorCounts - usedSectors;
        /* 大小都是以字节为单位进行保存 */
        superBlock->sectorBitmap.btmpBytesLen = freeSectors / 8;
        
        printk(PART_TIP "free sectors %d bitmap len %x\n", freeSectors,
            superBlock->sectorBitmap.btmpBytesLen);
        
        printk(PART_TIP "inode nr %d bitmap len %x\n", inodeNr,
            superBlock->inodeBitmap.btmpBytesLen);

        /* 写入超级块，保存所有修改的信息 */
        if (!BlockWrite(devno, part->startSector + 1, superBlock, 1)) {
            printk(PART_ERROR "device write failed!\n");
            return NULL;
        }

    } else {
        printk(PART_TIP "Fs has existed.\n");
        /*printk("BookOS file system super block:\n");
        printk("device id:%d sectors:%d\nsector bitmap lba:%d sector bitmap sectors:%d\n", 
            superBlock->devno, superBlock->part->sectorCounts, superBlock->sectorBitmapLba, 
            superBlock->sectorBitmapSectors);
        printk("inode bitmap lba:%d inode bitmap sectors:%d\ninode table lba:%d inode table sectors:%d data start lba:%d\n", 
            superBlock->inodeBitmapLba, superBlock->inodeBitmapSectors, 
            superBlock->inodeTableLba, superBlock->inodeTableSectors, 
            superBlock->dataStartLba);*/
    }
	return part->sb;
}


/**
 * InitFileSystem - 初始化文件系统
 * 
 */
PUBLIC void InitFileSystem()
{
	PART_START("File System");

	/* 获取块设备 */
	struct Disk *disk = GetDiskByName("hda");
	if (disk == NULL) {
		printk("Disk not found!\n");
	}
	
	DumpDisk(disk);
	
	DumpBlockDevice(disk->blkdev);
	
	DumpDiskPartition(&disk->part0);
	
	/* 获取磁盘的分区，将在分区上建立一个文件系统 */
	struct DiskPartition *part;

	/* 如果分区数大于0，那么就选取多个分区中的第一个分区，不然就选择0分区 */
	if (disk->minors > 0) {
		part = disk->part;
	} else {	/**/
		part = &disk->part0;
	}

	DumpDiskPartition(part);

	Spin("----fs----");
	struct BOFS_SuperBlock *sb;
	sb = BOFS_FormatOnPart(part, 4096);
	if (sb)
		BOFS_DumpSuperBlock(sb);
	/* 初始化BOFS */
	//BOFS_Init();
	
	/* 挂载第二个分区 */
	SysMakeDir("/mnt/c");

	SysMountDir("/dev/hda1", "/mnt/c");

	WriteDataToFS();

/*
	DIR *dir;
	DIRENT dirent;
	
	dir = SysOpenDir("/mnt/c");
	if (dir != NULL) {
		
		while (!SysReadDir(dir, &dirent)) {
			printk("dir name %s type %d inode %d size %d\n",
				dirent.name, dirent.type, dirent.inode, dirent.size);	
		}
	}*/
	/*
    int fd = SysOpen("/test2", O_CREAT | O_RDWR);
    if (fd < 0) {
        printk("file open failed!\n");
    }

    SysClose(fd);

	fd = SysOpen("/mnt/c/test", O_CREAT | O_RDWR);
    if (fd < 0) {
        printk("file open failed!\n");
    }

	char buf[32];
	memset(buf, 0x5a, 32); 
	if (SysWrite(fd, buf, 32) != 32) {
		printk("write failed!\n");
	}
	memset(buf, 0, 32);
	
	SysLseek(fd, 0, SEEK_SET);

	if (SysRead(fd, buf, 32) != 32) {
		printk("read failed!\n");
	}
	printk("%x %x\n", buf[0], buf[31]);
    SysClose(fd);

	SysMakeDir("/mnt/c/dir");*/
/*
	dir = SysOpenDir("/mnt/c");
	if (dir != NULL) {
		
		while (!SysReadDir(dir, &dirent)) {
			printk("dir name %s type %d inode %d size %d\n",
				dirent.name, dirent.type, dirent.inode, dirent.size);	
		}
	}*/
	/*
	if (SysRemoveDir("/mnt/c/dir")) {
		printk("remove dir failed!\n");
	}

	if (SysUnlink("/mnt/c/test")) {
		printk("unlink dir failed!\n");
	}*/

	PART_END();
}
