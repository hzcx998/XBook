/*
 * file:		include/driver/console.h
 * auther:		Jason Hu
 * time:		2019/6/26
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_CONSOLE_H
#define _DRIVER_CONSOLE_H

#include <share/stdint.h>
#include <share/types.h>
#include <book/chr-dev.h>

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

#define COLOR_DEFAULT	(MAKE_COLOR(TEXT_BLACK, TEXT_WHITE))

#define SCREEN_UP -1
#define SCREEN_DOWN 1

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

#define SCREEN_SIZE (80 * 25)

#define MAX_CONSOLE_BUF_SIZE (80*25)

/* 最多4个控制台 */
#define MAX_CONSOLE_NR	    CONSOLE_MINORS

/* 控制台结构 */
typedef struct Console {
    unsigned int currentStartAddr;      /* 当前显示到了什么位置 */
    unsigned int originalAddr;          /* 控制台对应的显存的位置 */
    unsigned int videoMemorySize;       /* 控制台占用的显存大小 */
    unsigned int cursor;                /* 当前光标的位置 */
    unsigned char color;                /* 字符的颜色 */

    /* 字符设备，由于初始化console的时候还没有内存分配，所以直接占用空间 */
    struct CharDevice chrdev;
} Console_t;

EXTERN Console_t consoleTable[];

void ConsoleInit();

enum {
    CON_CMD_SET_COLOR = 1,
    CON_CMD_SELECT_CON,
    CON_CMD_SCROLL,
    CON_CMD_CLEAN,
    CON_CMD_GOTO,
};

#endif   /*_DRIVER_CONSOLE_H*/
