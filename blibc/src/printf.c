/*
 * file:		user/lib/printf.c
 * auther:	    Jason Hu
 * time:		2019/8/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stdarg.h>
#include <share/vsprintf.h>
#include <user/conio.h>

/** 
 * printf - 格式化打印输出
 * @fmt: 格式以及字符串
 * @...: 可变参数
 */
int printf(const char *fmt, ...)
{
	//int i;
	char buf[256];
	va_list arg = (va_list)((char*)(&fmt) + 4); /*4是参数fmt所占堆栈中的大小*/
	vsprintf(buf, fmt, arg);
	
	log(buf);
	return 0;
}
