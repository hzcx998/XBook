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
#include <hal/char/display.h>

#define MAX_CONSOLE_BUF_SIZE (80*25)

void ConsoleInit();

//调试输出
PUBLIC int ConsolePrint(const char *fmt, ...);
PUBLIC int ConsoleFliter(char *buf, unsigned int count);

PUBLIC void ConsoleWrite(const char *buf, unsigned int count);
PUBLIC void ConsoleRead(const char *buf, unsigned int count);

PUBLIC void ConsoleGotoXY(unsigned short x, unsigned short y);
PUBLIC void ConsoleSetColor(unsigned char color);
PUBLIC void ConsoleReadGotoXY(unsigned short x, unsigned short y);

#endif   /*_DRIVER_CONSOLE_H*/
