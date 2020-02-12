/*
 * file:		include/char/console/console.h
 * auther:		Jason Hu
 * time:		2019/6/26
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_CONSOLE_H
#define _DRIVER_CONSOLE_H

#include <lib/stdint.h>
#include <lib/types.h>
#include <char/chr-dev.h>

/*
颜色生成方法
MAKE_COLOR(BLUE, RED)
MAKE_COLOR(BLACK, RED) | BRIGHT
MAKE_COLOR(BLACK, RED) | BRIGHT | FLASH
*/

#define TEXT_BLACK   0x0     /* 0000 */
#define TEXT_WHITE   0x7     /* 0111 */
#define TEXT_RED     0x4     /* 0100 */
#define TEXT_GREEN   0x2     /* 0010 */
#define TEXT_BLUE    0x1     /* 0001 */
#define TEXT_FLASH   0x80    /* 1000 0000 */
#define TEXT_BRIGHT  0x08    /* 0000 1000 */
#define	MAKE_COLOR(x,y)	((x<<4) | y) /* MAKE_COLOR(Background,Foreground) */

#define SCREEN_UP -1
#define SCREEN_DOWN 1

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

#define SCREEN_SIZE (80 * 25)

/* 最多4个控制台 */
#define MAX_CONSOLE_NR	    CONSOLE_MINORS

PUBLIC void ConsoleDebugPutChar(char ch);
void InitConsoleDebugDriver();

enum {
    CON_CMD_SET_COLOR = 1,
    CON_CMD_SELECT_CON,
    CON_CMD_SCROLL,
    CON_CMD_CLEAN,
    CON_CMD_GOTO,
};

#endif   /* _DRIVER_CONSOLE_H */
