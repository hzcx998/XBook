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
#include <book/list.h>

#define DEVICE_TYPE_BLOCK   0x01
#define DEVICE_TYPE_CHAR   0x02
#define DEVICE_TYPE_NET   0x04

/*
device 和 deviceio的区别。
device主要是记录设备存在信息，用于注册到/dev/目录下面，
如果有pointer，就可以进行挂载
deviceio主要是对设备的读写操作的抽象io
*/

struct Device {
    struct List list;           /* 所有设备构成一个设备链表 */
    char name[DEVICE_NAMELEN];  /* 记录设备名 */
    
    /* 记录该设备得设备id，通过设备ID可以对设备进行读写操作 */
    unsigned int deviceID;      
    
    char type; /* 设备类型 */
    /* 设备在内存空间对应的一个结构体指针，例如磁盘的一个分区 */
    void *pointer;
};

EXTERN struct List deviceListHead;

struct Device *CreateDevice(char *name, void *pointer, char type,
    unsigned int deviceID);

int MakeDevice(struct Device *device, char *name, void *pointer,
    char type, unsigned int deviceID);

int AddDevice(struct Device *device);
int DelDevice(struct Device *device);
int LookUpDevice(char *name);
void *GetDevicePointer(char *name);
void DumpDevices();
struct Device *GetDevice(char *name);



#endif   /* _BOOK_DEVICE_H */
