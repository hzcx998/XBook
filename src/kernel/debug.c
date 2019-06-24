/*
 * file:		kernel/hal.c
 * auther:	Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */


#include <book/debug.h>
#include <book/hal.h>

#include <share/vsprintf.h>

void InitKernelDebugHal()
{
   /*
	 * 初始化基于硬件抽象层的显示器
	 */
	HalInit(displayHal.hal);
   HalOpen(displayHal.hal);

   HalIoctl(displayHal.hal,DISPLAY_HAL_IO_CLEAR, 0);
   
   //设定打印函数指针
   printk = &DebugLog;
}

int DebugLog(const char *fmt, ...)
{
	int i;
	char buf[256];
	va_list arg = (va_list)((char*)(&fmt) + 4); /*4是参数fmt所占堆栈中的大小*/
	i = vsprintf(buf, fmt, arg);
	HalWrite(displayHal.hal,(unsigned char *)buf, i);

	return i;
}

PUBLIC void GotoXY(unsigned short x, unsigned short y)
{
	HalIoctl(displayHal.hal,DISPLAY_HAL_IO_CURSOR, y * SCREEN_WIDTH + x);
}

PUBLIC void DebugSetColor(unsigned char color)
{
	HalIoctl(displayHal.hal,DISPLAY_HAL_IO_COLOR, color);

}


