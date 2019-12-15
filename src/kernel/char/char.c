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

#include <drivers/keyboard.h>
#include <drivers/tty.h>

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
        return;
    }
	#endif
	
    /* 开启tty任务 */
    ThreadStart("tty", 3, TaskTTY, "NULL");

    PART_END();
}
