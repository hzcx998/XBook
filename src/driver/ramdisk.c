/*
 * file:		driver/ramdisk.c
 * auther:		Jason Hu
 * time:		2019/9/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <book/arch.h>
#include <book/config.h>
#include <book/debug.h>
#include <book/interrupt.h>
#include <driver/ramdisk.h>
#include <book/ioqueue.h>
#include <book/deviceio.h>
#include <share/string.h>
#include <book/task.h>
#include <share/math.h>
#include <share/const.h>
#include <book/vmarea.h>

/**
 * RAMDISK_Open - 打开硬盘
 * @deviceEntry: 设备项
 * @name: 设备名
 * @mode: 打开模式
 */
PRIVATE int RAMDISK_Open(struct DeviceEntry *deviceEntry, char *name, char *mode)
{
	
	return 0;
}

/**
 * RAMDISK_Close - 关闭硬盘
 * @deviceEntry: 设备项
 */
PRIVATE int RAMDISK_Close(struct DeviceEntry *deviceEntry)
{
	
	return 0;
}

/**
 * RAMDISK_Ioctl - 硬盘的IO控制
 * @deviceEntry: 设备项
 * @cmd: 命令
 * @arg1: 参数1
 * @arg2: 参数2
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int RAMDISK_Ioctl(struct DeviceEntry *deviceEntry, int cmd, int arg1, int arg2)
{

	return 0;
}

/**
 * RAMDISK_Read - 读取硬盘数据
 * @deviceEntry: 设备项
 * @buf: 读取到缓冲区
 * @count: 操作的扇区数
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int RAMDISK_Read(struct DeviceEntry *deviceEntry, unsigned int lba, void *buffer, unsigned int count)
{

	return 0;
}

/**
 * RAMDISK_Write - 写入数据到硬盘
 * @deviceEntry: 设备项
 * @buf: 要写入的缓冲区
 * @count: 操作的扇区数
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int RAMDISK_Write(struct DeviceEntry *deviceEntry, unsigned int lba, void *buffer, unsigned int count)
{

	return 0;
}

/**
 * RAMDISK_Init - 初始化RAMDISK硬盘驱动
 * @deviceEntry: 设备项
 */
PRIVATE int RAMDISK_Init(struct DeviceEntry *deviceEntry)
{

	return 0;
}

/**
 * 设备的操作控制
 */
PRIVATE struct DeviceOperate operate = {
	.Init =  &RAMDISK_Init, 
	.Open = &RAMDISK_Open, 
	.Close = &RAMDISK_Close, 
	.Read = &RAMDISK_Read, 
	.Write = &RAMDISK_Write,
	.Getc = (void *)&IoNull, 
	.Putc = (void *)&IoNull, 
	.Ioctl = &RAMDISK_Ioctl, 
};

/**
 * InitRamdisk_Driver - 初始化ramdisk硬盘驱动
 */
PUBLIC void InitRamdisk_Driver()
{
	PART_START("RAMDISK Driver");

    PART_END();
}
