/*
 * file:		char/virtual/tty.c
 * auther:		Jason Hu
 * time:		2019/8/8
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/debug.h>
#include <book/device.h>
#include <book/signal.h>
#include <lib/string.h>
#include <lib/vsprintf.h>
#include <lib/stddef.h>
#include <char/chr-dev.h>

#define DRV_NAME "null"

struct NullPrivate {
    CharDevice_t *chrdev;   /* 字符设备 */
} nullPrivate;

/**
 * NullRead - 空设备读 
 */
PRIVATE int NullGetc(struct Device *device)
{
    return 0;
}

/**
 * NullWrite - 空设备写
 */
PRIVATE int NullPutc(struct Device *device, unsigned int ch)
{
    //printk("null:%c", ch);
    return 0;
}

/* 设备操作 */
PRIVATE struct DeviceOperations nullOpSets = {
	.getc = NullGetc,
	.putc = NullPutc,
};

PUBLIC int InitNullDeviceDriver()
{
    /* 设置一个字符设备号 */
    nullPrivate.chrdev = AllocCharDevice(DEV_NULL);
	if (nullPrivate.chrdev == NULL) {
		printk(PART_ERROR "alloc char device for tty failed!\n");
		return -1;
	}
        
	/* 初始化字符设备信息 */
	CharDeviceInit(nullPrivate.chrdev, 1, &nullPrivate);
	CharDeviceSetup(nullPrivate.chrdev, &nullOpSets);

	CharDeviceSetName(nullPrivate.chrdev, DRV_NAME);
	/* 把字符设备添加到系统 */
	AddCharDevice(nullPrivate.chrdev);

    /*
    DeviceOpen(DEV_NULL, 0);

    DevicePutc(DEV_NULL, '$');

    DeviceClose(DEV_NULL);*/

    return 0;
}
