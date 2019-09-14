/*
 * file:		kernel/deviceio.c
 * auther:	    Jason Hu
 * time:		2019/8/24
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/string.h>
#include <book/debug.h>
#include <book/deviceio.h>
#include <book/arch.h>
#include <driver/keyboard.h>

/* 设备表 */
struct DeviceEntry deviceTable[MAX_DEVICE_NR];

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
 * IsBadDevice - 检测是否为错误的设备
 * @deviceID: 设备的id
 * 
 * 如果是错误id就返回1，否则返回0
 */
PRIVATE int IsBadDevice(int deviceID)
{
    if (0 <= deviceID && deviceID < MAX_DEVICE_NR) {
        return 0;
    }
    return 1;
}

/**
 * IdToDeviceEntry - 通过设备ID获取设备项
 * deviceID: 设备id
 * 
 * 如果以后不用数组来转换，直接修改这里面就行了
 */
PRIVATE struct DeviceEntry *IdToDeviceEntry(int deviceID)
{
    return &deviceTable[deviceID];
}

/**
 * DeviceCheckID_OK - 检测ID是否正确
 * @deviceID: 设备的id
 * 
 * 检测id无误，并且已经注册，那么就可以进行操作
 */
PUBLIC int DeviceCheckID_OK(int deviceID)
{
    if (IsBadDevice(deviceID))
        return -1;

    struct DeviceEntry *deviceEntry = IdToDeviceEntry(deviceID);

    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (deviceID != MAKE_DEVICE_ID(deviceEntry->major, deviceEntry->minor))
        return -1;

    return 0;
}

/**
 * DeviceGetMinor - 获取设备的从设备号
 * @deviceID: 设备id
 * 
 */
PUBLIC int DeviceGetMinor(int deviceID)
{
    if (IsBadDevice(deviceID))
        return -1;

    struct DeviceEntry *deviceEntry = IdToDeviceEntry(deviceID);

    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (deviceID != MAKE_DEVICE_ID(deviceEntry->major, deviceEntry->minor))
        return -1;

    return deviceEntry->minor;
}

/**
 * RegisterDeviceIO - 注册设备I/O抽象
 * @major: 主设备号
 * @minor: 次设备号
 * @name: 设备名字
 * @operate: 设备执行的操作 
 * @device: 设备特有信息
 * @handler: 设备中断处理
 * @irq: 设备irq号
 * 
 * 注册一个设备I/O抽象。
*/
PUBLIC int RegisterDevice(
    int major,
    int minor,
    char *name, 
    struct DeviceOperate *operate,
    void *device,
    void (*handler)(unsigned int, unsigned int),
    char irq)
{
    int deviceID = MAKE_DEVICE_ID(major, minor);

    /* 如果设备号出错就直接返回 */
    if (IsBadDevice(deviceID))
        return -1;

    struct DeviceEntry *deviceEntry;
    
    /* 注册设备的时候不能产生中断 */
    enum InterruptStatus oldStatus = InterruptDisable();

    /* 获取设备项并注册信息 */
    deviceEntry = IdToDeviceEntry(deviceID);
    deviceEntry->major = major;
    deviceEntry->minor = minor;
    deviceEntry->name = name;
    deviceEntry->operate = operate;
    deviceEntry->device = device;
    deviceEntry->Handler = handler;
    deviceEntry->irq = irq;
    
    AtomicSet(&deviceEntry->references, 0); 

    InterruptSetStatus(oldStatus);
    return 0;
}

/**
 * DeviceLookOver - 查看设备状态
 * @deviceID: 要查看的设备的id
 */
PUBLIC void DeviceLookOver(int deviceID)
{
    /* 如果设备号出错就直接返回 */
    if (IsBadDevice(deviceID))
        return;

    struct DeviceEntry *deviceEntry;
    
    /* 获取设备项并获取操作 */
    deviceEntry = IdToDeviceEntry(deviceID);

    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (deviceID != MAKE_DEVICE_ID(deviceEntry->major, deviceEntry->minor))
        return;

    printk(PART_TIP "----Device Look Over----\n");

    printk(PART_TIP "Name:%s Major:%d Minor:%d ID:%d\n", 
            deviceEntry->name, deviceEntry->major, deviceEntry->minor, 
            MAKE_DEVICE_ID(deviceEntry->major, deviceEntry->minor));

    printk(PART_TIP "Device:%x Handler:%x IRQ:%d\n", 
            deviceEntry->device, deviceEntry->Handler, deviceEntry->irq, 
            MAKE_DEVICE_ID(deviceEntry->major, deviceEntry->minor));

}
/**
 * DeviceRead - 设备的读取操作
 * @deviceID: 设备id号
 * @buffer: 缓冲区
 * @count: 扇区数
 */
PUBLIC int DeviceRead(int deviceID, unsigned int lba, void *buffer, unsigned int count)
{
    struct DeviceEntry *deviceEntry;
    int retval;

    /* 检测是否是坏设备 */
    if (IsBadDevice(deviceID))
        return -1;

    deviceEntry = IdToDeviceEntry(deviceID);
    
    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (deviceID != MAKE_DEVICE_ID(deviceEntry->major, deviceEntry->minor))
        return -1;

    /* 没有打开设备就直接返回 */
    if (!AtomicGet(&deviceEntry->references))
        return -1;

    retval = (*deviceEntry->operate->Read)(deviceEntry, lba, buffer, count);

    return retval;
}

/**
 * DeviceWrite - 设备的写入操作
 * @deviceID: 设备id号
 * @buffer: 缓冲区
 * @count: 字节数
 */
PUBLIC int DeviceWrite(int deviceID, unsigned int lba, void *buffer, unsigned int count)
{
    struct DeviceEntry *deviceEntry;
    int retval;

    /* 检测是否是坏设备 */
    if (IsBadDevice(deviceID))
        return -1;

    deviceEntry = IdToDeviceEntry(deviceID);
    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (deviceID != MAKE_DEVICE_ID(deviceEntry->major, deviceEntry->minor))
        return -1;
    
    /* 没有打开设备就直接返回 */
    if (!AtomicGet(&deviceEntry->references))
        return -1;

    retval = (*deviceEntry->operate->Write)(deviceEntry, lba, buffer, count);

    return retval;
}

/**
 * DeviceIoctl - 设备的控制操作
 * @deviceID: 设备id号
 * @cmd: 命令
 * @arg1: 参数1
 * @arg2: 参数2
 * 
 */
PUBLIC int DeviceIoctl(int deviceID, int cmd, int arg1, int arg2)
{
    struct DeviceEntry *deviceEntry;
    int retval;

    /* 检测是否是坏设备 */
    if (IsBadDevice(deviceID))
        return -1;

    deviceEntry = IdToDeviceEntry(deviceID);
    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (deviceID != MAKE_DEVICE_ID(deviceEntry->major, deviceEntry->minor))
        return -1;
    
    /* 没有打开设备就直接返回 */
    if (!AtomicGet(&deviceEntry->references))
        return -1;

    retval = (*deviceEntry->operate->Ioctl)(deviceEntry, cmd, arg1, arg2);

    return retval;
}

/**
 * DeviceIoctl - 设备的获取字符操作
 * @deviceID: 设备id号
 * 
 * 从设备获取一个字符
 */
PUBLIC int DeviceGetc(int deviceID)
{
    struct DeviceEntry *deviceEntry;
    int retval;

    /* 检测是否是坏设备 */
    if (IsBadDevice(deviceID))
        return -1;

    deviceEntry = IdToDeviceEntry(deviceID);
    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (deviceID != MAKE_DEVICE_ID(deviceEntry->major, deviceEntry->minor))
        return -1;
    
    /* 没有打开设备就直接返回 */
    if (!AtomicGet(&deviceEntry->references))
        return -1;

    retval = (*deviceEntry->operate->Getc)(deviceEntry);

    return retval;
}


/**
 * DevicePutc - 向设备的发送字符操作
 * @deviceID: 设备id号
 * @ch: 字符
 * 
 * 向设备发送一个字符
 */
PUBLIC int DevicePutc(int deviceID, char ch)
{
    struct DeviceEntry *deviceEntry;
    int retval;

    /* 检测是否是坏设备 */
    if (IsBadDevice(deviceID))
        return -1;

    deviceEntry = IdToDeviceEntry(deviceID);
    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (deviceID != MAKE_DEVICE_ID(deviceEntry->major, deviceEntry->minor))
        return -1;
    
    /* 没有打开设备就直接返回 */
    if (!AtomicGet(&deviceEntry->references))
        return -1;

    retval = (*deviceEntry->operate->Putc)(deviceEntry, ch);

    return retval;
}

/**
 * DeviceOpen - 打开设备
 * @deviceID: 设备id号
 * @name: 设备名
 * @mode: 模式
 * 
 */
PUBLIC int DeviceOpen(int deviceID, char *name, char *mode)
{
    struct DeviceEntry *deviceEntry;
    int retval = 0;

    /* 检测是否是坏设备 */
    if (IsBadDevice(deviceID))
        return -1;

    deviceEntry = IdToDeviceEntry(deviceID);
    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (deviceID != MAKE_DEVICE_ID(deviceEntry->major, deviceEntry->minor))
        return -1;
    
    /* 增加引用 */
    if (AtomicGet(&deviceEntry->references) >= 0)
        AtomicInc(&deviceEntry->references);
    else 
        return -1;  /* 引用计数有错误 */

    /* 是第一次引用才打开 */
    if (AtomicGet(&deviceEntry->references) == 1)
        retval = (*deviceEntry->operate->Open)(deviceEntry, name, mode);
    
    return retval;
}


/**
 * DeviceClose - 关闭设备
 * 
 */
PUBLIC int DeviceClose(int deviceID)
{
    struct DeviceEntry *deviceEntry;
    int retval = 0;

    /* 检测是否是坏设备 */
    if (IsBadDevice(deviceID))
        return -1;

    deviceEntry = IdToDeviceEntry(deviceID);
    /* 如果传入的ID和注册的不一致就直接返回(用于检测没有注册但是使用) */
    if (deviceID != MAKE_DEVICE_ID(deviceEntry->major, deviceEntry->minor))
        return -1;
    
    /* 增加引用 */
    if (AtomicGet(&deviceEntry->references) > 0)
        AtomicDec(&deviceEntry->references);
    else 
        return -1;  /* 引用计数有错误 */

    /* 是最后一次引用才关闭 */
    if (AtomicGet(&deviceEntry->references) == 0)
        retval = (*deviceEntry->operate->Close)(deviceEntry);
    
    return retval;
}

/**
 * DeviceInit - 设备初始化
 * @deviceID: 设备id号
 */
PUBLIC int DeviceInit(int deviceID)
{
    struct DeviceEntry *deviceEntry;
    int retval;

    /* 检测是否是坏设备 */
    if (IsBadDevice(deviceID))
        return -1;
    
    deviceEntry = IdToDeviceEntry(deviceID);
    
    retval = (*deviceEntry->operate->Init)(deviceEntry);

    return retval;
}


/**
 * InitDeviceIO - 初始化设备io
 */
PUBLIC void InitDeviceIO()
{
    PART_START("Device IO");
    /* 清空设备项 */
    memset(deviceTable, 0, sizeof(struct DeviceEntry) *MAX_DEVICE_NR);
    
    /*
    RegisterDevice(DEVICE_CONSOLE, 0, "console", NULL, NULL, NULL, 0);
    
    RegisterDevice(DEVICE_KEYBOARD, 0, "keyboard", NULL, NULL, NULL, 1);
    
    DeviceLookOver(DEVICE_CONSOLE);    
    DeviceLookOver(DEVICE_KEYBOARD);
    */

    //Spin("InitDeviceIO");
    PART_END();
}