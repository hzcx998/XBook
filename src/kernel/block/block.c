/*
 * file:	    kernel/block/block.c
 * auther:	    Jason Hu
 * time:	    2019/10/13
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/debug.h>
#include <share/string.h>
#include <book/block.h>
#include <book/device.h>
#include <book/task.h>
#include <driver/clock.h>
#include <driver/ramdisk.h>
#include <driver/ide.h>

#include <book/blk-buffer.h>
#include <book/blk-request.h>
#include <book/blk-dev.h>
#include <book/blk-disk.h>

//#define _DEBUG_TEST

EXTERN struct List allDiskList;
EXTERN struct List allBlockDeviceList;

/**
 * ThreadDiskFlush - 磁盘刷新线程
 * @arg: 参数
 * 
 * 每隔1s进行一次磁盘同步
 */
void ThreadDiskFlush(void *arg)
{
    int count;
    while (1) {
        /* 每隔1s同步一次 */
        TaskSleep(1*HZ);

        /* 执行块同步操作 */
        count = Bsync();
        if (count) {
            printk(">>>sync disk for %d count.\n", count);
        }
        count = 0;
    }
}

void ThreadReadTest(void *arg)
{
    char *blkbuf = kmalloc(BLOCK_SIZE, GFP_KERNEL);
    if (blkbuf == NULL) {
        Panic("alloc blkbuf failed!\n");
    }
    int i = 0;
    char dir = 1;
    struct BufferHead *wbh;

    while (1) {
        i += dir;

        if (i > 5) {
            dir = -dir;
        } 
        if (i <= 0) {
            dir = -dir;
        }

        wbh = Bread(DEV_RDA, i);
        if (wbh == NULL)
            printk("read failed!\n");
    }
}

void ThreadReadTest2(void *arg)
{
    char *blkbuf = kmalloc(BLOCK_SIZE, GFP_KERNEL);
    if (blkbuf == NULL) {
        Panic("alloc blkbuf failed!\n");
    }
    int i = 5;
    char dir = 1;
    struct BufferHead *wbh;

    while (1) {
        i += dir;

        if (i > 10) {
            dir = -dir;
        } 
        if (i <= 5) {
            dir = -dir;
        }
        memset(blkbuf, i, BLOCK_SIZE);
        wbh = Bwrite(DEV_RDA, i, blkbuf);

        wbh = Bread(DEV_RDA, i);
        if (wbh == NULL)
            printk("read failed!\n");
    }
}

void ThreadReadTest3(void *arg)
{
    char *blkbuf = kmalloc(BLOCK_SIZE, GFP_KERNEL);
    if (blkbuf == NULL) {
        Panic("alloc blkbuf failed!\n");
    }
    int i = 5;
    char dir = -1;
    struct BufferHead *wbh;

    while (1) {
        i += dir;
        
        if (i > 5) {
            dir = -dir;
        }
        if (i <= 0) {
            dir = -dir;
        }
        
        memset(blkbuf, i, BLOCK_SIZE);
        wbh = Bwrite(DEV_HDB, i, blkbuf);

        wbh = Bread(DEV_RDA, i);
        if (wbh == NULL)
            printk("read failed!\n");
    }
}

/**
 * BlockDeviceTest - 对块设备进行测试
 * 
 */
PUBLIC void BlockDeviceTest()
{
    #ifdef _DEBUG_TEST
    ThreadStart("A", 3, ThreadReadTest, "NULL");
    ThreadStart("B", 3, ThreadReadTest2, "NULL");
    ThreadStart("C", 3, ThreadReadTest3, "NULL");
    
    struct BufferHead *bh = Bread(DEV_RDA, 0);
    if (bh == NULL)
        Panic("read bh failed!\n");
    DumpBH(bh);

    printk("data:%x-%x-%x-%x\n", bh->data[0], bh->data[511], bh->data[512], bh->data[1023]);
    
    bh = Bread(DEV_RDA, 0);
    if (bh == NULL) 
        Panic("read bh failed!\n");
    DumpBH(bh);

    printk("data:%x-%x-%x-%x\n", bh->data[0], bh->data[511], bh->data[512], bh->data[1023]);
    //Panic("test\n");

    char *blkbuf = kmalloc(BLOCK_SIZE, GFP_KERNEL);
    if (blkbuf == NULL) {
        Panic("alloc blkbuf failed!\n");
    }
    
    memset(blkbuf, 0XFA, BLOCK_SIZE);

    struct BufferHead *wbh;
    wbh = Bwrite(DEV_RDB, 0, blkbuf);
    if (bh == NULL)
        Panic("write bh failed!\n");
    DumpBH(wbh);

    printk("data:%x-%x-%x-%x\n", wbh->data[0], wbh->data[511], wbh->data[512], wbh->data[1023]);

    wbh = Bwrite(DEV_RDB, 1, blkbuf);
    if (bh == NULL)
        Panic("write bh failed!\n");
    DumpBH(wbh);

    printk("data:%x-%x-%x-%x\n", wbh->data[0], wbh->data[511], wbh->data[512], wbh->data[1023]);
    #endif
}

/**
 * InitBlockDevice - 初始化块设备层
 */
PUBLIC void InitBlockDevice()
{
    PART_START("BlockDevice");
    
    /* 创建一个线程来同步磁盘 */
    ThreadStart("dflush", 3, ThreadDiskFlush, "NULL");

    /* 初始化ramdisk驱动 */
    if (InitRamdiskDriver()) {
		Panic("init ramdisk failed!\n");	
	}
    
    /* 初始化ide驱动 */
    /*if (InitIdeDriver()) {
		Panic("init ide failed!\n");	
	}*/

    DeviceOpen(DEV_RDA, 0);

    DeviceIoctl(DEV_RDA, RAMDISK_IO_CLEAN, 10);

    char *buf = kmalloc(SECTOR_SIZE * 2, GFP_KERNEL);
    if (buf == NULL) {
        return;
    }

    buf[0] = 0x11;
    buf[511] = 0x55;
    buf[512] = 0xaa;
    buf[1023] = 0xff;
    DeviceWrite(DEV_RDA, 0, buf, 2);
    memset(buf, 0, 1024);

    DeviceRead(DEV_RDA, 0, buf, 2);

    printk("buf:%x %x %x %x\n", buf[0], buf[511], buf[512], buf[1023]);
    
    BlockDeviceTest();
    
    /* 打印磁盘，并打印分区 */
    /*struct Disk *disk;

    ListForEachOwner(disk, &allDiskList, list) {
        //DumpDisk(disk);
    }
    struct BlockDevice *blkdev;
    
    ListForEachOwner(blkdev, &allBlockDeviceList, list) {
        DumpBlockDevice(blkdev);
        //DumpDiskPartition(blkdev->part);
    }*/
    PART_END();
}
