/*
 * file:	    kernel/char/chrdev.c
 * auther:	    Jason Hu
 * time:	    2019/10/13
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/debug.h>
#include <share/string.h>
#include <book/char.h>
#include <book/chr-dev.h>
#include <book/memcache.h>
#include <book/arch.h>

EXTERN struct List allDeviceListHead;

/* 所有字符设备的队列 */
LIST_HEAD(allCharDeviceList);

/**
 * GetCharDeviceByDevno - 通过设备号获取字符设备
 * @devno: 设备号
 * 
 * 返回字符设备
 */
PUBLIC struct CharDevice *GetCharDeviceByDevno(dev_t devno)
{
    struct CharDevice *chrdev;

    ListForEachOwner(chrdev, &allCharDeviceList, list) {
        if (chrdev->super.devno == devno) {
            return chrdev;
        }
    }
    return NULL;
}

PUBLIC void DumpCharDevice(struct CharDevice *chrdev)
{
    printk(PART_TIP "----Char Device----\n");

    printk(PART_TIP "devno:%x name:%s private:%x \n",
        chrdev->super.devno, chrdev->super.name, chrdev->private);

}

/**
 * AllocCharDevice - 分配一个字符设备
 * @devno: 表示字符设备号
 * 
 * 分配一个和字符设备号对应的字符设备
 */
PUBLIC struct CharDevice *AllocCharDevice(dev_t devno)
{
    struct CharDevice *device = kmalloc(SIZEOF_CHAR_DEVICE, GFP_KERNEL);
    if (device == NULL)
        return NULL;
    
    device->super.devno = devno;
    
    return device;
}

/**
 * AllocCharDevice - 分配一个字符设备
 * @devno: 表示字符设备号
 * 
 * 分配一个和字符设备号对应的字符设备
 */
PUBLIC void CharDeviceSetDevno(struct CharDevice *chrdev, dev_t devno)
{
    chrdev->super.devno = devno;
}

/**
 * CharDeviceInit - 对一个字符设备进行初始化
 * @chrdev: 字符设备
 * @count: 一个类别的设备有多少个设备
 * @private: 私有数据，创建者可以使用的区域
 */
PUBLIC void CharDeviceInit(struct CharDevice *chrdev,
    unsigned int count,
    void *private)
{
    chrdev->private = private;
    chrdev->count = count;

    /* 设备的私有数据就是字符设备的地址 */
    chrdev->super.private = chrdev;

    /* 标识成字符设备 */
    chrdev->super.type = DEV_TYPE_CHAR;
}

/**
 * CharDeviceSetup - 对一个字符设备进行调整
 * @chrdev: 字符设备
 * @opSets: 操作集
 */
PUBLIC void CharDeviceSetup(struct CharDevice *chrdev,
    struct DeviceOperations *opSets)
{
    chrdev->super.opSets = opSets;
}

/**
 * AddCharDevice - 把字符设备添加到内核
 * @chrdev: 要添加的字符设备
 */
PUBLIC void AddCharDevice(struct CharDevice *chrdev)
{
    /* 添加到字符设备队列的最后面 */
    ListAddTail(&chrdev->list, &allCharDeviceList);

    /* 添加到设备队列的最后面 */
    ListAddTail(&chrdev->super.list, &allDeviceListHead);

    return;
}

/**
 * DelCharDevice - 删除一个字符设备
 * @chrdev: 字符设备
 * 
 * 删除字符设备，就是从内核中除去对它的描述
 */
PUBLIC void DelCharDevice(struct CharDevice *chrdev)
{
    ListDel(&chrdev->list);

    ListDel(&chrdev->super.list);
}

/**
 * FreeCharDevice - 释放一个字符设备
 * @chrdev: 字符设备
 * 
 * 释放字符符设备占用的内存
 */
PUBLIC void FreeCharDevice(struct CharDevice *chrdev)
{
    if (chrdev) 
        kfree(chrdev);
}

/**
 * CharDeviceSetName - 设置字符设备的名字
 * @chrdev: 字符设备
 * @name: 名字
 * 
 */
PUBLIC void CharDeviceSetName(struct CharDevice *chrdev, char *name)
{
    memset(chrdev->super.name, 0, DEVICE_NAME_LEN);
    strcpy(chrdev->super.name, name);
}
