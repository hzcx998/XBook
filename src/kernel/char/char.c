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

#include <drivers/keyboard.h>
#include <drivers/tty.h>
#include <drivers/serial.h>

#define _DEBUG_TEST

EXTERN struct List allCharDeviceList;

/**
 * InitCharDevice - 初始化字符设备层
 */
PUBLIC void InitCharDevice()
{
    PART_START("CharDevice");
    
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

    /* 开启tty任务 */
    ThreadStart("tty", 3, TaskTTY, "NULL");

    /* 初始化音频系统 */
    InitSoundSystem();
    
    PART_END();
}
