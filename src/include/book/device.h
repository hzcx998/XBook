/*
 * file:		include/book/device.h
 * auther:		Jason Hu
 * time:	    2019/9/11
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_DEVICE_H
#define _BOOK_DEVICE_H

#include <book/list.h>
#include <book/atomic.h>

typedef unsigned int dev_t;

enum {
    DEV_TYPE_BLOCK,
    DEV_TYPE_CHAR,
    DEV_TYPE_NET,
    MAX_DEV_TYPE_NR
};

/* 这里学习linux的做法，高12位是主设备号，低20位是从设备号 */
#define MINORBITS 20
#define MINORMASK 0x000fffff

#define MKDEV(major,minor) (((major) << MINORBITS) | (minor))

#define MAJOR(dev) ((dev) >> MINORBITS)
#define MINOR(dev) ((dev) & MINORMASK)

/* 设备号 */
#define CONSOLE_MAJOR      1
#define KEYBOARD_MAJOR     2
#define IDE_MAJOR          3
#define RAMDISK_MAJOR      4

/* 设备分区数 */
#define IDE_PARTS          8
#define RAMDISK_PARTS      8

/* 从设备号 */
#define IDE_MINOR(x)        (x * (IDE_PARTS + 1))
#define RAMDISK_MINOR(x)    (x * (RAMDISK_PARTS + 1)) 

/* 设备号合成 */

#define DEV_CONSOLE         MKDEV(CONSOLE_MAJOR, 0)       /* 控制台设备 */
#define DEV_KEYBOARD        MKDEV(KEYBOARD_MAJOR, 0)       /* 键盘设备 */

#define DEV_HDA             MKDEV(IDE_MAJOR, IDE_MINOR(0))       /* IDE硬盘：主通道0磁盘 */
#define DEV_HDB             MKDEV(IDE_MAJOR, IDE_MINOR(1))       /* IDE硬盘：主通道1磁盘 */
#define DEV_HDC             MKDEV(IDE_MAJOR, IDE_MINOR(2))       /* IDE硬盘：从通道0磁盘 */
#define DEV_HDD             MKDEV(IDE_MAJOR, IDE_MINOR(3))       /* IDE硬盘：从通道1磁盘 */

#define DEV_RDA            MKDEV(RAMDISK_MAJOR, RAMDISK_MINOR(0))       /* RAMDISK设备 */
#define DEV_RDB            MKDEV(RAMDISK_MAJOR, RAMDISK_MINOR(1))       /* RAMDISK设备 */

#define DEVICE_NAME_LEN 24

/* 设备的抽象
每个设备子系统都应该继承这个抽象，然后再添加设备自己有的属性
 */
struct Device {
    struct List list;                   /* 所有设备构成一个设备链表 */
    dev_t devno;                        /* 设备号 */
    char name[DEVICE_NAME_LEN];         /* 设备名 */
    struct DeviceOperations *opSets;    /* 设备操作集 */
    char type;                          /* 设备类型 */
    void *private;                      /* 指向设备子系统（字符设备，块设备） */
    Atomic_t references;                /* 设备的引用计数 */
};

struct DeviceOperations {
    int (*open)(struct Device *, unsigned int);
    int (*close)(struct Device *);
    int (*read)(struct Device *, unsigned int, void *, unsigned int);
    int (*write)(struct Device *, unsigned int, void *, unsigned int);
    int (*ioctl)(struct Device *, int, int);  
    int (*getc)(struct Device *);
    int (*putc)(struct Device *, unsigned int);
};

#define SIZEOF_DEVICE sizeof(struct Device)

PUBLIC int SearchDevice(char *name);
PUBLIC void DumpDevices();
PUBLIC struct Device *GetDeviceByName(char *name);
PUBLIC struct Device *GetDeviceByID(int devno);

PUBLIC void DumpDevice(struct Device *device);

/* 空操作或者错误操作 */
PUBLIC int IoNull();
PUBLIC int IoError();

/* 设备操作集接口 */
PUBLIC int DeviceOpen(int devno, unsigned int flags);
PUBLIC int DeviceClose(int devno);
PUBLIC int DeviceIoctl(int devno, int cmd, int arg);
PUBLIC int DeviceWrite(int devno, unsigned int lba, void *buffer, unsigned int count);
PUBLIC int DeviceRead(int devno, unsigned int lba, void *buffer, unsigned int count);
PUBLIC int DeviceGetc(int devno);
PUBLIC int DevicePutc(int devno, unsigned int ch);

#endif   /* _BOOK_DEVICE_H */
