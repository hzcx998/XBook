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

/*
color set

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

#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)

#define MAX_CONSOLE_BUF_SIZE (80*25)

void ConsoleInit();

//调试输出
PUBLIC int ConsolePrint(const char *fmt, ...);
PUBLIC int ConsoleFliter(char *buf, unsigned int count);

PUBLIC void ConsoleWrite(char *buf, unsigned int count);
PUBLIC void ConsoleRead(char *buf, unsigned int count);

PUBLIC void ConsoleGotoXY(unsigned short x, unsigned short y);
PUBLIC void ConsoleSetColor(unsigned char color);
PUBLIC void ConsoleReadGotoXY(unsigned short x, unsigned short y);

PUBLIC int SysLog(char *buf);

#endif   /*_DRIVER_CONSOLE_H*/
