/*
 * file:	    char/core/char.c
 * auther:	    Jason Hu
 * time:	    2019/10/13
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/debug.h>
#include <book/device.h>
#include <book/task.h>
#include <sound/sound.h>
#include <char/char.h>
#include <lib/string.h>
#include <book/kgc.h>

/* ----驱动程序初始化文件导入---- */
EXTERN int InitSerialDriver();
EXTERN int InitConsoleDriver();
EXTERN int InitNullDeviceDriver();
EXTERN int InitGttyDriver();

/* ----驱动程序初始化文件导入完毕---- */

#define _DEBUG_TEST

EXTERN struct List allCharDeviceList;

PRIVATE void InitCharDrivers()
{
    if (InitNullDeviceDriver()) {
        printk("init null device driver failed!\n");
    }

#ifdef CONFIG_DRV_CONSOLE
    if (InitConsoleDriver()) {
        printk("init console driver failed!\n");
    }
#endif  /* CONFIG_DRV_CONSOLE */

#ifdef CONFIG_DRV_SERIAL
    if (InitSerialDriver()) {
        printk("init serial driver failed!\n");
    }
#endif  /* CONFIG_DRV_SERIAL */
    if (InitGttyDriver()) {
        printk("init serial driver failed!\n");
    }
}

/**
 * InitCharDevice - 初始化字符设备层
 */
PUBLIC void InitCharDevice()
{
    InitCharDrivers();
}
