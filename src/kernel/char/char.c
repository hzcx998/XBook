/*
 * file:	    kernel/block/block.c
 * auther:	    Jason Hu
 * time:	    2019/10/13
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/debug.h>
#include <share/string.h>
#include <book/device.h>
#include <book/char.h>

#include <driver/keyboard.h>

#define _DEBUG_TEST

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
		
		key = DeviceGetc(DEV_KEYBOARD);
		if (key != KEYCODE_NONE)
			printk("%c", key); 
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
    
	/* 初始化键盘驱动 */
	if (InitKeyboardDriver()) {
        return;
    }
	


    CharDeviceTest();
    
    PART_END();
}
