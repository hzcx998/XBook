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
LIST_HEAD(allDeviceListHead);


/*
构建一个散列表，用来把设备号和结构进行对应，可以提高
对设备的搜索速度
*/

/**
 * LookUpDevice - 查找一个设备
 * @name: 设备名
 * 
 * 找到返回1，没有返回0
 */
PUBLIC int SearchDevice(char *name)
{
    struct Device *device;
    ListForEachOwner(device, &allDeviceListHead, list) {
        /* 如果名字相等就说明找到 */
        if (!strcmp(device->name, name)) {
            return 1;
        }
    }
    return 0;
}

/**
 * GetDeviceByName - 获取设备
 * @name: 设备名
 * 
 * 返回设备结构体
 */
PUBLIC struct Device *GetDeviceByName(char *name)
{
    struct Device *device;
    ListForEachOwner(device, &allDeviceListHead, list) {
        /* 如果名字相等就说明找到 */
        if (!strcmp(device->name, name)) {
            return device;
        }
    }
    return NULL;
}

PUBLIC void DumpDevice(struct Device *device)
{
    printk(PART_TIP "----Devices----\n");
    printk(PART_TIP "name:%s devno:%x type:%d ops:%x\n",
        device->name, device->devno, device->type, device->opSets);
    printk(PART_TIP "private:%x ref:%d\n",
        device->private, device->references);
}

/**
 * DumpDevices - 打印所有设备信息
 */
PUBLIC void DumpDevices()
{
    printk(PART_TIP "----Devices----\n");
    struct Device *device;
    ListForEachOwner(device, &allDeviceListHead, list) {
        DumpDevice(device);
    }
}

/**
 * IsBadDevice - 检测是否为错误的设备
 * @devno: 设备的id
 * 
 * 如果是错误id就返回1，否则返回0
 */
PRIVATE int IsBadDevice(int devno)
{
    
    return 0;
}

/**
 * GetDeviceByIDEntry - 通过设备ID获取设备项
 * devno: 设备id
 * 
 * 如果以后不用数组来转换，直接修改这里面就行了
 */
PUBLIC struct Device *GetDeviceByID(int devno)
{
    /* 先用散列表的形式搜索 */


    /* 再用链表搜索的方式 */
    struct Device *device;
    ListForEachOwner(device, &allDeviceListHead, list) {
        if (device->devno == devno) {
            return device;
        }
    }
    return NULL;
}

/**
 * IoNull - 空的操作
 * 
 * 返回成功，表示啥也不做
 */
PUBLIC int IoNull()
{
    return 0;
}

/**
 * IoError - 错误的操作
 * 
 * 返回失败
 */
PUBLIC int IoError()
{
    return -1;
}

/**
 * DeviceRead - 设备的读取操作
 * @devno: 设备id号
 * @buffer: 缓冲区
 * @count: 扇区数
 */
PUBLIC int DeviceRead(int devno, unsigned int lba, void *buffer, unsigned int count)
{
    struct Device *device;
    int retval = -1;

    /* 检测是否是坏设备 */
    if (IsBadDevice(devno))
        return -1;

    device = GetDeviceByID(devno);
    
    if (device == NULL)
        return -1;

    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (devno != device->devno)
        return -1;

    /* 没有打开设备就直接返回 */
    if (!AtomicGet(&device->references))
        return -1;
    if (device->opSets->read != NULL)
        retval = (*device->opSets->read)(device, lba, buffer, count);

    return retval;
}

/**
 * DeviceWrite - 设备的写入操作
 * @devno: 设备id号
 * @buffer: 缓冲区
 * @count: 字节数
 */
PUBLIC int DeviceWrite(int devno, unsigned int lba, void *buffer, unsigned int count)
{
    struct Device *device;
    int retval = -1;

    /* 检测是否是坏设备 */
    if (IsBadDevice(devno))
        return -1;

    device = GetDeviceByID(devno);
    
    if (device == NULL)
        return -1;

    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (devno != device->devno)
        return -1;
    /* 没有打开设备就直接返回 */
    if (!AtomicGet(&device->references))
        return -1;
    if (device->opSets->write != NULL)
        retval = (*device->opSets->write)(device, lba, buffer, count);

    return retval;
}

/**
 * DeviceIoctl - 设备的控制操作
 * @devno: 设备id号
 * @cmd: 命令
 * @arg: 参数1
 * 
 */
PUBLIC int DeviceIoctl(int devno, int cmd, int arg)
{
    struct Device *device;
    int retval = -1;

    /* 检测是否是坏设备 */
    if (IsBadDevice(devno))
        return -1;

    device = GetDeviceByID(devno);

    if (device == NULL)
        return -1;

    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (devno != device->devno)
        return -1;
    
    /* 没有打开设备就直接返回 */
    if (!AtomicGet(&device->references))
        return -1;
    if (device->opSets->ioctl != NULL)
        retval = (*device->opSets->ioctl)(device, cmd, arg);

    return retval;
}

/**
 * DeviceOpen - 打开设备
 * @devno: 设备id号
 * @name: 设备名
 * @mode: 模式
 * 
 */
PUBLIC int DeviceOpen(int devno, unsigned int flags)
{
    struct Device *device;
    int retval = 0;

    /* 检测是否是坏设备 */
    if (IsBadDevice(devno))
        return -1;

    device = GetDeviceByID(devno);

    if (device == NULL)
        return -1;

    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (devno != device->devno)
        return -1;
    
    /* 增加引用 */
    if (AtomicGet(&device->references) >= 0)
        AtomicInc(&device->references);
    else 
        return -1;  /* 引用计数有错误 */

    /* 是第一次引用才打开 */
    if (AtomicGet(&device->references) == 1) {
        if (device->opSets->open != NULL)
            retval = (*device->opSets->open)(device, flags);
        
    }
        
    return retval;
}

/**
 * DeviceClose - 关闭设备
 */
PUBLIC int DeviceClose(int devno)
{
    struct Device *device;
    int retval = 0;

    /* 检测是否是坏设备 */
    if (IsBadDevice(devno))
        return -1;

    device = GetDeviceByID(devno);

    if (device == NULL)
        return -1;

    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (devno != device->devno)
        return -1;
    /* 增加引用 */
    if (AtomicGet(&device->references) > 0)
        AtomicDec(&device->references);
    else 
        return -1;  /* 引用计数有错误 */

    //printk("dev %x, ref %d\n", device->devno, AtomicGet(&device->references));
    
    /* 是最后一次引用才关闭 */
    if (AtomicGet(&device->references) == 0) {
        if (device->opSets->close != NULL)
            retval = (*device->opSets->close)(device);
        
    }
     
    return retval;
}

/**
 * DeviceGetc - 设备获取一个字符
 * @devno: 设备id号
 */
PUBLIC int DeviceGetc(int devno)
{
    struct Device *device;
    int retval = -1;

    /* 检测是否是坏设备 */
    if (IsBadDevice(devno))
        return -1;

    device = GetDeviceByID(devno);
    
    if (device == NULL)
        return -1;

    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (devno != device->devno)
        return -1;

    /* 没有打开设备就直接返回 */
    if (!AtomicGet(&device->references))
        return -1;
    if (device->opSets->getc != NULL)
        retval = (*device->opSets->getc)(device);

    return retval;
}

/**
 * DevicePutc - 向设备输入一个字符
 * @devno: 设备id号
 * @ch: 字符
 */
PUBLIC int DevicePutc(int devno, unsigned int ch)
{
    struct Device *device;
    int retval = -1;

    /* 检测是否是坏设备 */
    if (IsBadDevice(devno))
        return -1;

    device = GetDeviceByID(devno);
    
    if (device == NULL)
        return -1;

    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (devno != device->devno)
        return -1;
    /* 没有打开设备就直接返回 */
    if (!AtomicGet(&device->references))
        return -1;
    if (device->opSets->putc != NULL)
        retval = (*device->opSets->putc)(device, ch);

    return retval;
}
