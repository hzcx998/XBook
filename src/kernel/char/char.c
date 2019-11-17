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

#include <drivers/keyboard.h>

//#define _DEBUG_TEST

EXTERN struct List allCharDeviceList;

/**
 * CharDeviceTest - 对字符设备进行测试
 * 
 */
PUBLIC void CharDeviceTest()
{
    PART_START("Test");
    #ifdef _DEBUG_TEST
    
	int key = 0;

	DeviceOpen(DEV_KEYBOARD, 0);

	DeviceIoctl(DEV_KEYBOARD, KEYBOARD_CMD_MODE, KEYBOARD_MODE_SYNC);

	while (1) {
		key = 0;
		if (!DeviceRead(DEV_KEYBOARD, 0, &key, 1)) {
			printk("%c", key); 
		}
	};


    #endif
    PART_END();
}

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
	
    CharDeviceTest();
    
    PART_END();
}
