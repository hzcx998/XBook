/*
 * file:		kernel/lib/debug.c
 * auther:		Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/debug.h>
#include <book/arch.h>
#include <lib/vsprintf.h>

#include <char/uart/serial.h>
#include <char/console/console.h>

//停机并输出大量信息
void Panic(const char *fmt, ...)
{
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	vsprintf(buf, fmt, arg);

    //DeviceIoctl(DEV_CON0, CON_CMD_SET_COLOR, MAKE_COLOR(TEXT_GREEN,TEXT_BLACK));
    
	printk("\n> Panic: %s", buf);
	DisableInterrupt();
	while(1){
		CpuHlt();
	}

}
//断言
void AssertionFailure(char *exp, char *file, char *baseFile, int line)
{

	//DeviceIoctl(DEV_CON0, CON_CMD_SET_COLOR, MAKE_COLOR(TEXT_RED,TEXT_BLACK));
    
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

    int count = i;
    char *p = buf;

    while (count-- > 0) {
        ConsoleDebugPutChar(*p++);
    }

	return i;
}

/**
 * SerialPrint - 串口格式化输出
 * @fmt: 格式
 * @...: 参数
 * 
 * 返回缓冲区长度
 */
PUBLIC int SerialPrint(const char *fmt, ...)
{
	int i;
	char buf[256];
	va_list arg = (va_list)((char*)(&fmt) + 4); /*4是参数fmt所占堆栈中的大小*/
	i = vsprintf(buf, fmt, arg);

    int count = i;
    char *p = buf;
    while (count-- > 0) {
        SerialDebugPutChar(*p++);
    }
    
	return i;
}

PUBLIC void DebugColor(unsigned int color)
{
	//DeviceIoctl(DEV_CON0, CON_CMD_SET_COLOR, color);
}

/**
 * InitDebugPrint - 初始化调试打印
 * 
 */
PUBLIC void InitDebugPrint()
{
#ifdef CONFIG_CONSOLE_DEBUG
    // 初始化控制台
	InitConsoleDebugDriver();
    printk = &ConsolePrint;
#endif /* CONFIG_CONSOLE_DEBUG */

#ifdef CONFIG_SERIAL_DEBUG
    // 初始化串口驱动
    InitSerialDebugDriver();
    printk = &SerialPrint;
    
#endif /* CONFIG_SERIAL_DEBUG */
} 
