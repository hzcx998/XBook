/*
 * file:		include/book/device.h
 * auther:		Jason Hu
 * time:	    2019/9/11
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_DEVICE_H
#define _BOOK_DEVICE_H


#include <book/deviceio.h>
#include <book/bitmap.h>

/*
device 和 deviceio的区别。
device主要是记录设备存在信息，用于注册到/dev/目录下面，
如果有pointer，就可以进行挂载
deviceio主要是对设备的读写操作的抽象io
*/

struct Device {
    struct List list;           /* 所有设备构成一个设备链表 */
    char name[DEVICE_NAMELEN];  /* 记录设备名 */
    
    /* 设备在内存空间对应的一个结构体指针，例如磁盘的一个分区 */
    void *pointer;
};

struct Device *CreateDevice(char *name, void *pointer);
int MakeDevice(struct Device *device, char *name, void *pointer);
int AddDevice(struct Device *device);
int DelDevice(struct Device *device);
int LookUpDevice(char *name);
void *GetDevicePointer(char *name);
void DumpDevices();

#endif   /* _BOOK_DEVICE_H */
