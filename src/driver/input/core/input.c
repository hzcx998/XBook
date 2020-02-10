/*
 * file:		input/core/input.c
 * auther:		Jason Hu
 * time:		2020/2/10
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/debug.h>
#include <input/input.h>
//#include <input/keyboard/ps2.h>
//#include <input/mouse/ps2.h>

/* ----驱动程序初始化文件导入---- */
EXTERN int InitPs2KeyboardDriver();
EXTERN int InitPs2MouseDriver();
EXTERN int InitTTYDriver();
/* ----驱动程序初始化文件导入完毕---- */

PRIVATE void InitInputDrivers()
{
    
#ifdef CONFIG_DRV_KEYBOARD
	/* 初始化键盘驱动 */
	if (InitPs2KeyboardDriver()) {
        printk("init keyboard driver failed!\n");
    }
#endif  /* CONFIG_DRV_KEYBOARD */
    
#ifdef CONFIG_DRV_MOUSE
    if (InitPs2MouseDriver()) {
        printk("init tty driver failed!\n");
    }
#endif  /* CONFIG_DRV_MOUSE */
        
#ifdef CONFIG_DRV_TTY
    if (InitTTYDriver()) {
        printk("init tty driver failed!\n");
    }
#endif  /* CONFIG_DRV_TTY */

}

/**
 * InitInputSystem - 初始化输入系统
 */
PUBLIC int InitInputSystem()
{
    InitInputDrivers();

    return 0;
}
