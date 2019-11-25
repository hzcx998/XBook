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
#include <drivers/ide.h>
#include <share/vsprintf.h>
#include <share/const.h>
#include <book/blk-buffer.h>
#include <book/blk-dev.h>


#include <fs/partition.h>
#include <fs/bofs/super_block.h>
#include <fs/bofs/inode.h>
#include <fs/bofs/dir_entry.h>
#include <fs/bofs/bitmap.h>
#include <fs/bofs/dir.h>
#include <fs/bofs/file.h>
#include <fs/bofs/device.h>

#include <fs/vfs.h>

EXTERN struct List allDeviceListHead;

PRIVATE void BOFS_Test()
{

    return;
    //Spin("test");

    /*
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
    */

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


/**
 * BOFS_ProbeFileSystem - 探测分区是否已经有文件系统
 * @part: 分区
 * 
 * 成功返回文件系统超级块指针
 * 失败返回NULL
 */
PRIVATE VFS void *BOFS_ProbeFileSystem(struct DiskPartition *part)
{
    struct Disk *disk = part->disk;
    
    struct BOFS_SuperBlock *superBlock = kmalloc(disk->blkdev->blockSize, GFP_KERNEL);
    if (superBlock == NULL) {
        printk("kmalloc for superBlock Failed!\n");
        return NULL;
    }
    
    /* 读取超级块 */
    if (BlockRead(MKDEV(disk->major, disk->firstMinor), 1, superBlock)) {
        Panic("block read failed!\n");
    }

    if (BOFS_HAD_FS(superBlock)) {
        return superBlock;
    } else {
        return NULL;
    }
}

/**
 * BOFS_MakeFileSystem - 在分区上创建文件系统
 * @part: 分区
 * 
 * 成功返回文件系统超级块指针
 * 失败返回NULL
 */
PRIVATE VFS void *BOFS_MakeFileSystem(struct DiskPartition *part)
{
    struct Disk *disk = part->disk;
    
    struct BOFS_SuperBlock *superBlock = kmalloc(disk->blkdev->blockSize, GFP_KERNEL);
    if (superBlock == NULL) {
        printk("kmalloc for superBlock Failed!\n");
        return NULL;
    }

    if (BOFS_MakeFS(superBlock,
        MKDEV(disk->major, disk->firstMinor), 
        part->startSector,
        part->sectorCounts,
        disk->blkdev->blockSize,
        DEFAULT_MAX_INODE_NR)) 
    {
        printk(PART_WARRING "make book file system on disk failed!\n");
        return NULL;
    }
    return superBlock;
}

/**
 * BOFS_MountFileSystem - 挂载文件系统到内存
 * @part: 分区
 * 
 * 成功返回文件系统超级块指针
 * 失败返回NULL
 */
PRIVATE VFS int BOFS_MountFileSystem(struct BOFS_SuperBlock *superBlock)
{
    if (BOFS_MountFS(superBlock)) {
        printk(PART_ERROR "mount bofs failed!\n");
        return -1;
    }
    
    if (BOFS_OpenRootDir(superBlock)) {
        printk(PART_ERROR "open dir for bofs failed!\n");
        return -1;
    }

    return 0;
}
/* BOFS文件系统的操作集 */
PRIVATE struct FileSystemOperations fsOpSets = {
    .probeFileSystem = &BOFS_ProbeFileSystem,
    .makeFileSystem = &BOFS_MakeFileSystem,
    .mountFileSystem = &BOFS_MountFileSystem,
};

/* 定义一个文件系统 */
PRIVATE DEFINE_FILE_SYSTEM(bofs);

PUBLIC int InitBoFS()
{
    PART_START("Bofs");

    /* 添加文件系统到内核 */
    VFS_AddFileSystem(&bofs, "bofs", &fsOpSets);

    /* 初始化文件系统的目录 */
    BOFS_InitDir();
    
    /* 初始化文件系统的文件 */
    BOFS_InitFile();

    /* 获取设备文件 */
	

    
    //DumpDiskPartition(part);
/*
    struct BOFS_SuperBlock *superBlock = kmalloc(disk->blkdev->blockSize, GFP_KERNEL);
    if (!superBlock)
        Panic("kmalloc for superBlock Failed!\n");
    */
    /* 读取超级块 */
    /*if (BlockRead(MKDEV(disk->major, disk->firstMinor), 1, superBlock)) {
        Panic("block read failed!\n");
    }

    if (BOFS_HAD_FS(superBlock)) {
        printk("disk had fs.");
    } else {
        BOFS_MakeFS(superBlock,
        MKDEV(disk->major, disk->firstMinor), 
        part->startSector,
        part->sectorCounts,
        disk->blkdev->blockSize,
        DEFAULT_MAX_INODE_NR);
    }*/

    /* 挂载文件系统，从磁盘上加载数据到内存中 */
    //BOFS_MountFS(superBlock);
    //printk("ready to open root dir!\n");
    /* 打开根目录 */
    //BOFS_OpenRootDir(superBlock);




    //BOFS_DumpSuperBlock(superBlock);
    /*
    BOFS_MakeDirSub("/test", superBlock);
    BOFS_MakeDirSub("/test/bin", superBlock);
    BOFS_MakeDirSub("/test/dd", superBlock);
    
    struct BOFS_Dir *dir = BOFS_OpenDir2("/test", superBlock);
    if (dir == NULL) {
        printk("open dir failed!\n");
    }
    struct BOFS_DirEntry *de = BOFS_ReadDir(dir);
    while (de != NULL) {
        BOFS_DumpDirEntry(de);

        de = BOFS_ReadDir(dir);
    }

    BOFS_RemoveDirSub("/test/dd", superBlock);
    
    if (BOFS_ResetName("/test/bin", "share", superBlock))
        printk("rename failed!");

    dir = BOFS_OpenDir2("/test", superBlock);
    if (dir == NULL) {
        printk("open dir failed!\n");
    }
    de = BOFS_ReadDir(dir);
    while (de != NULL) {
        BOFS_DumpDirEntry(de);

        de = BOFS_ReadDir(dir);
    }
    
    BOFS_CloseDir(dir);
    */
    /*
    int fd = BOFS_Open("/bar",BOFS_O_CREAT | BOFS_O_RDWR, superBlock);
    if (fd == -1)
        printk("file open failed!\n");

    char buf[32];

    memset(buf, 0, 32);
    strcpy(buf,"hello, world!\n");

    int wr = BOFS_Write(fd, buf, 32);
    if (wr == -1)
        printk("file write failed!\n");

    printk("write buf %d\n", wr);

    memset(buf , 0, 32);

    BOFS_Lseek(fd, 0, BOFS_SEEK_SET);

    int rd = BOFS_Read(fd, buf, 32);
    if (rd == -1)
        printk("file read failed!\n");

    printk("buf: %s\n", buf);

    BOFS_Close(fd);

    if (BOFS_Access("/bar", BOFS_F_OK, superBlock)) {
        printk("file not exist!\n");
    }

    printk("file mode:%x", BOFS_GetMode("/bar", superBlock));
    printk("file set mode:%d", BOFS_SetMode("/bar", BOFS_IMODE_W, superBlock));
    printk("file mode:%x", BOFS_GetMode("/bar", superBlock));
    
    struct BOFS_Stat fstat;

    BOFS_Stat("/bar",
        &fstat,
        superBlock);

    printk("stat: size %d type %d mode %x\n", fstat.size, fstat.type, fstat.mode);

    if (BOFS_Unlink("/bar", superBlock)) {
        printk("unlink file failed!\n");
    }
*/




    PART_END();
    return 0;
}
