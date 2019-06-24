/*
 * file:		   include/book/hal.h
 * auther:		Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_DEBUG_H
#define _BOOK_DEBUG_H

#include <share/stdint.h>
#include <share/types.h>
#include <hal/hal.h>

void InitKernelDebugHal();

//调试输出
int DebugLog(const char *fmt, ...);
PUBLIC void GotoXY(unsigned short x, unsigned short y);
PUBLIC void DebugSetColor(unsigned char color);

//内核打印函数的指针
int (*printk)(const char *fmt, ...);

#endif   /*_BOOK_DEBUG_H*/
