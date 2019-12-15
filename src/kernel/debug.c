/*
 * file:		kernel/debug.c
 * auther:		Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */


#include <book/debug.h>
#include <drivers/console.h>
#include <share/vsprintf.h>
#include <book/arch.h>

//停机并输出大量信息
void Panic(const char *fmt, ...)
{
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	vsprintf(buf, fmt, arg);

    DeviceIoctl(DEV_CON0, CON_CMD_SET_COLOR, MAKE_COLOR(TEXT_GREEN,TEXT_BLACK));
    
	printk("\n> Panic: %s", buf);
	DisableInterrupt();
	while(1){
		CpuHlt();
	}

}
//断言
void AssertionFailure(char *exp, char *file, char *baseFile, int line)
{

	DeviceIoctl(DEV_CON0, CON_CMD_SET_COLOR, MAKE_COLOR(TEXT_RED,TEXT_BLACK));
    
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
	DisableInterrupt();
	while(1){
		CpuHlt();
	}
}

/**
 * ConsolePrint - 控制台格式化输出
 * @fmt: 格式
 * @...: 参数
 * 
 * 返回缓冲区长度
 */
PUBLIC int ConsolePrint(const char *fmt, ...)
{
	int i;
	char buf[256];
	va_list arg = (va_list)((char*)(&fmt) + 4); /*4是参数fmt所占堆栈中的大小*/
	i = vsprintf(buf, fmt, arg);

	//ConsoleWrite((char *)buf, i);

    DeviceWrite(DEV_CON0, 0, buf, i);

	return i;
}

PUBLIC void DebugColor(unsigned int color)
{
	DeviceIoctl(DEV_CON0, CON_CMD_SET_COLOR, color);
}

/**
 * InitDebugPrint - 初始化调试打印
 * 
 */
PUBLIC void InitDebugPrint()
{
    /* 先打开第一个控制台，用来调试输出 */
    DeviceOpen(DEV_CON0, 0);
    
    printk = &ConsolePrint;
} 
