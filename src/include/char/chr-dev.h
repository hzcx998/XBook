/*
 * file:		include/char/chr-dev.h
 * auther:		Jason Hu
 * time:		2019/10/25
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _CHAR_DEVICE_H
#define _CHAR_DEVICE_H

#include <lib/types.h>
#include <book/list.h>
#include <book/device.h>

/**
 * 字符设备是每一个字符流设备的输入输出的抽象
 */
typedef struct CharDevice {
    struct Device super;        /* 继承设备抽象 */
    struct List list;           /* 连接所有字符设备的链表 */

    unsigned int count;         /* 同一主设备号的次设备号的个数 */
    void *private;              /* 私有数据，一般指向驱动中的设备接口 */
} CharDevice_t;

#define SIZEOF_CHAR_DEVICE sizeof(struct CharDevice)

PUBLIC struct CharDevice *AllocCharDevice(dev_t devno);
PUBLIC void CharDeviceSetDevno(struct CharDevice *chrdev, dev_t devno);
PUBLIC void CharDeviceInit(struct CharDevice *chrdev,
    unsigned int count,
    void *private);
PUBLIC void CharDeviceSetup(struct CharDevice *chrdev,
    struct DeviceOperations *opSets);
PUBLIC void AddCharDevice(struct CharDevice *chrdev);
PUBLIC void DelCharDevice(struct CharDevice *chrdev);
PUBLIC void FreeCharDevice(struct CharDevice *chrdev);
PUBLIC void CharDeviceSetName(struct CharDevice *chrdev, char *name);

PUBLIC void DumpCharDevice(struct CharDevice *chrdev);

PUBLIC struct CharDevice *GetCharDeviceByDevno(dev_t devno);

#endif   /* _CHAR_DEVICE_H */
