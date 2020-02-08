/*
 * file:	    kernel/block/block.c
 * auther:	    Jason Hu
 * time:	    2019/10/13
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/debug.h>
#include <share/string.h>
#include <book/device.h>
#include <book/char.h>
#include <book/task.h>
#include <book/sound.h>

#include <system/graph.h>

#include <drivers/keyboard.h>
#include <drivers/tty.h>
#include <drivers/serial.h>
#include <drivers/mouse.h>

#define _DEBUG_TEST

EXTERN struct List allCharDeviceList;

PRIVATE void InitCharDrivers()
{
#ifdef CONFIG_DRV_KEYBOARD
	/* 初始化键盘驱动 */
	if (InitKeyboardDriver()) {
        printk("init keyboard driver failed!\n");
    }
#endif  /* CONFIG_DRV_KEYBOARD */
    
#ifdef CONFIG_DRV_SERIAL
    if (InitSerialDriver()) {
        printk("init serial driver failed!\n");
    }
#endif  /* CONFIG_DRV_SERIAL */
    
#ifdef CONFIG_DRV_TTY
    if (InitTTYDriver()) {
        printk("init tty driver failed!\n");
    }
#endif  /* CONFIG_DRV_TTY */
    

#ifdef CONFIG_DRV_MOUSE
    if (InitMouseDriver()) {
        printk("init tty driver failed!\n");
    }
#endif  /* CONFIG_DRV_MOUSE */
    
}

/**
 * InitCharDevice - 初始化字符设备层
 */
PUBLIC void InitCharDevice()
{
    PART_START("CharDevice");
    
    InitCharDrivers();
    /* 初始化音频系统 */
    InitSoundSystem();
    /* 初始化图形系统 */
    InitGraphSystem();

    PART_END();
}
