/*
 * file:		kernel/device.c
 * auther:	    Jason Hu
 * time:		2019/9/11
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/share.h>
#include <book/debug.h>
#include <book/arch.h>
#include <book/device.h>

/* 设备链表 */
LIST_HEAD(deviceListHead);

/**
 * CreateDevice - 创建一个设备
 * @name: 设备名
 * @pointer: 特殊指针
 */
PUBLIC struct Device *CreateDevice(char *name, void *pointer)
{
    struct Device *device = kmalloc(sizeof(struct Device), GFP_KERNEL);
    if (device == NULL)
        return NULL;

    INIT_LIST_HEAD(&device->list);

    memset(device->name, 0, DEVICE_NAMELEN);
    strcpy(device->name, name);

    device->pointer = pointer;

    return device;
}

/**
 * MakeDevice - 生成设备信息
 * @device: 要生成的设备
 * @name: 设备名
 * @pointer: 特殊指针
 */
PUBLIC int MakeDevice(struct Device *device, char *name, void *pointer)
{
    INIT_LIST_HEAD(&device->list);

    memset(device->name, 0, DEVICE_NAMELEN);
    strcpy(device->name, name);

    device->pointer = pointer;
}

/**
 * AddDevice - 添加一个设备
 * @device: 要添加的设备
 */
PUBLIC int AddDevice(struct Device *device)
{
    /* 设备已经存在 */
    if (ListFind(&device->list, &deviceListHead))
        return -1;

    /* 添加到链表 */
    ListAddTail(&device->list, &deviceListHead);
    return 0;
}

/**
 * DelDevice - 删除一个设备
 * @device: 要删除的设备
 */
PUBLIC int DelDevice(struct Device *device)
{
    /* 设备不在链表中就返回 */
    if (!ListFind(&device->list, &deviceListHead))
        return -1;

    /* 添加到链表 */
    ListDel(&device->list);
    return 0;
}

/**
 * LookUpDevice - 查找一个设备
 * @name: 设备名
 * 
 * 找到返回1，没有返回0
 */
PUBLIC int LookUpDevice(char *name)
{
    struct Device *device;
    ListForEachOwner(device, &deviceListHead, list) {
        /* 如果名字相等就说明找到 */
        if (!strcmp(device->name, name)) {
            return 1;
        }
    }
    return 0;
}

/**
 * GetDevicePointer - 获取设备的指针
 * @name: 设备名
 */
PUBLIC void *GetDevicePointer(char *name)
{
    struct Device *device;
    ListForEachOwner(device, &deviceListHead, list) {
        /* 如果名字相等就说明找到 */
        if (!strcmp(device->name, name)) {
            return device->pointer;
        }
    }
    return NULL;
}

/**
 * DumpDevices - 打印所有设备信息
 */
PUBLIC void DumpDevices()
{
    printk(PART_TIP "----Devices----\n");
    struct Device *device;
    ListForEachOwner(device, &deviceListHead, list) {
        printk(PART_TIP "name:%s pointer:%x\n", device->name, device->pointer);
        BOFS_DumpSuperBlock(device->pointer);
    }
}
