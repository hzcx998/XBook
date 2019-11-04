/*
 * file:		include/fs/device.h
 * auther:		Jason Hu
 * time:		2019/11/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _FS_DEVICE_H
#define _FS_DEVICE_H

#include <share/stdint.h>
#include <share/types.h>
#include <book/device.h>

#include <fs/file.h>

/**
 * 设备文件
 */
struct DeviceFile {
    struct File super;  /* 继承文件 */

    struct List list;               /* 设备文件在设备目录下面的链表 */
    struct Directory *directory;    /* 设备文件对应的设备目录指针 */
    struct Device *device;          /* 设备文件对应的设备指针 */
};
#define SIZEOF_DEVICE_FILE sizeof(struct DeviceFile)


PUBLIC struct File *CreateFile(char *name, char type, char attr);
PUBLIC int MakeDeviceFile(char *dname,
	char *fname,
	char type,
	struct Device *device);
PUBLIC void DumpDeviceFile(struct DeviceFile *devfile);
PUBLIC struct DeviceFile *GetDeviceFileByName(struct List *deviceList, char *name);
struct DeviceFile *CreateDeviceFile(char *name,
	char attr, struct Directory *dir, struct Device *device);
PUBLIC int DeviceFileRead( struct FileDescriptor *fdptr,
	void* buf,
	unsigned int count);
PUBLIC int DeviceFileWrite( struct FileDescriptor *fdptr,
	void* buf,
	unsigned int count);

#endif	/* _FS_DEVICE_H */

