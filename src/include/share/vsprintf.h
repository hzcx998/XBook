/*
 * file:		include/share/vsprintf.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _SHARE_VSPRINTF_H
#define _SHARE_VSPRINTF_H

#include "../share/stdarg.h"

#define STR_DEFAULT_LEN 256

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);
int printf(const char *fmt, ...);

#endif  /*_SHARE_VSPRINTF_H*/
