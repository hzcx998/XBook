/*
 * file:		printf.c
 * auther:	    Jason Hu
 * time:		2019/8/2
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <stdarg.h>
#include <vsprintf.h>
#include <string.h>
#include <conio.h>
#include <stdio.h>

/** 
 * printf - 格式化打印输出
 * @fmt: 格式以及字符串
 * @...: 可变参数
 */
int printf(const char *fmt, ...)
{
	//int i;
	char buf[STR_DEFAULT_LEN];
	va_list arg = (va_list)((char*)(&fmt) + 4); /*4是参数fmt所占堆栈中的大小*/
	vsprintf(buf, fmt, arg);
	
    /* 写入到标准输出 */
    write(1, buf, strlen(buf));
	return 0;
}
