/*
 * file:		kernel/debug.c
 * auther:		Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */


#include <book/debug.h>
#include <driver/console.h>
#include <share/vsprintf.h>
#include <book/arch.h>

//停机并输出大量信息
void Panic(const char *fmt, ...)
{
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	vsprintf(buf, fmt, arg);

    ConsoleSetColor(MAKE_COLOR(TEXT_GREEN,TEXT_BLACK));
	
	printk("\n> Panic: %s", buf);
	DisableInterrupt();
	while(1);

}
//断言
void AssertionFailure(char *exp, char *file, char *baseFile, int line)
{

    ConsoleSetColor(MAKE_COLOR(TEXT_RED,TEXT_BLACK));
	
	printk("\nassert(%s) failed:\nfile: %s\nbase_file: %s\nln%d",
	exp, file, baseFile, line);

	Spin("> AssertionFailure()");

	/* should never arrive here */
        __asm__ __volatile__("ud2");
}
//停机显示函数名
void Spin(char * functionName)
{
	printk("\nSpinning in %s", functionName);
	while(1);
}
