/*
 * file:		include/lib/conio.h
 * auther:		Jason Hu
 * time:		2019/7/29
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _USER_LIB_CONIO_H
#define _USER_LIB_CONIO_H

#include <drivers/keyboard.h>
#include <drivers/tty.h>
#include <drivers/clock.h>

void log(char *buf);
int printf(const char *fmt, ...);

#endif  /*_USER_LIB_CONIO_H*/
