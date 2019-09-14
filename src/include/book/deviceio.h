/*
 * file:		include/book/deviceio.h
 * auther:		Jason Hu
 * time:	    2019/8/24
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_DEVICEIO_H
#define _BOOK_DEVICEIO_H

#include <share/types.h>
#include <book/atomic.h>

typedef unsigned int devid_t;

/*
设备io抽象接口抽象
*/

#define DEVICE_CONSOLE      0       /* 控制台设备 */
#define DEVICE_KEYBOARD     1       /* 键盘设备 */
#define DEVICE_HDD0         2       /* ATA硬盘：主通道0磁盘 */
#define DEVICE_HDD1         3       /* ATA硬盘：主通道1磁盘 */
#define DEVICE_HDD2         4       /* ATA硬盘：从通道0磁盘 */
#define DEVICE_HDD3         5       /* ATA硬盘：从通道1磁盘 */

#define DEVICE_HDD_NR       4       /* ATA硬盘数 */

/* 通过major和minor生成device id */
#define MAKE_DEVICE_ID(major, minor)    (major + minor)


/* 最大的设备数量 */
#define MAX_DEVICE_NR  6

/* 设备名长度 */
#define DEVICE_NAMELEN  24

/* 设备项 */
struct DeviceEntry {
    int major;      /* 设备主id号 */
    int minor;      /* 设备次id号 */
    char *name;     /* 设备名 */

    /* 设备的操作 */
    struct DeviceOperate *operate;    

    void *device;    /* 设备特有的信息结构，控制和状态寄存器 */
    void (*Handler)(unsigned int, unsigned int);
    char irq;       /* 设备的irq号 */

    /* 内置成员 */
	Atomic_t references;     /* 设备的引用计数 */
};

/* 设备对应的操作 */
struct DeviceOperate {
    int (*Init)(struct DeviceEntry *);
    int (*Open)(struct DeviceEntry *, char *, char *);
    int (*Close)(struct DeviceEntry *);
    int (*Read)(struct DeviceEntry *, unsigned int, void *, unsigned int);
    int (*Write)(struct DeviceEntry *, unsigned int, void *, unsigned int);
    int (*Getc)(struct DeviceEntry *);
    int (*Putc)(struct DeviceEntry *, char);
    int (*Ioctl)(struct DeviceEntry *, int, int, int);  
};

EXTERN struct DeviceEntry DeviceTable[];

PUBLIC void InitDeviceIO();

PUBLIC int RegisterDevice(
    int major,
    int minor,
    char *name, 
    struct DeviceOperate *operate,
    void *device,
    void (*handler)(unsigned int, unsigned int),
    char irq);
PUBLIC void DeviceLookOver(int deviceID);
PUBLIC int DeviceGetMinor(int deviceID);
PUBLIC int DeviceCheckID_OK(int deviceID);

PUBLIC int IoNull();
PUBLIC int IoError();

PUBLIC int DeviceOpen(int deviceID, char *name, char *mode);
PUBLIC int DeviceClose(int deviceID);
PUBLIC int DevicePutc(int deviceID, char ch);
PUBLIC int DeviceGetc(int deviceID);
PUBLIC int DeviceIoctl(int deviceID, int cmd, int arg1, int arg2);
PUBLIC int DeviceWrite(int deviceID, unsigned int lba, void *buffer, unsigned int count);
PUBLIC int DeviceRead(int deviceID, unsigned int lba, void *buffer, unsigned int count);
PUBLIC int DeviceInit(int deviceID);

#endif   /* _BOOK_DEVICEIO_H */
