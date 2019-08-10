/*
 * file:		driver/char/console.c
 * auther:		Jason Hu
 * time:		2019/6/26
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */


#include <book/debug.h>
#include <book/hal.h>
#include <share/vsprintf.h>
#include <hal/char/display.h>
#include <book/arch.h>
#include <share/string.h>
#include <book/slab.h>

PUBLIC void ConsoleInit()
{
   	/*
	初始化基于硬件抽象层的显示器
	*/
   	HalOpen("display");
   	HalIoctl("display",DISPLAY_HAL_IO_CLEAR, 0);
	
   	//设定打印函数指针
   	printk = &ConsolePrint;

	filterk = &ConsoleFliter;
}

PUBLIC int ConsolePrint(const char *fmt, ...)
{
	int i;
	char buf[256];
	va_list arg = (va_list)((char*)(&fmt) + 4); /*4是参数fmt所占堆栈中的大小*/
	i = vsprintf(buf, fmt, arg);
	HalWrite("display",(unsigned char *)buf, i);

	return i;
}
/*
 * ConsoleFliter - 控制台信息过滤
 * @buf: 过滤到哪个部分
 * @count: 要求过滤多少数据
 * 
 * 返回实际过滤到的字符数量
 * 注：它需要和ConsoleReadGotoXY一起使用，才能达到最好的效果
 */
PUBLIC int ConsoleFliter(char *buf, unsigned int count)
{
	//申请一样大的内存来暂时存放
	unsigned char *tmp = kmalloc(PAGE_SIZE);
	
	//整个读取过来
	HalRead("display",tmp, count);

	//过滤出非0的字符
	int i = 0, j = 0;
	while (j < count)
	{
		if (tmp[j] != '\0') {
			buf[i] = tmp[j];
			i++;
		}
		j++;
	}
	//释放
	kfree(tmp);

	//在buf的最后添加一个0，表示字符结束
	buf[count - 1] = '\0';
	return i;
}

PUBLIC void ConsoleWrite(const char *buf, unsigned int count)
{
	HalWrite("display",(unsigned char *)buf, count);
}

PUBLIC void ConsoleRead(const char *buf, unsigned int count)
{
	HalRead("display",(unsigned char *)buf, count);
}

PUBLIC void ConsoleGotoXY(unsigned short x, unsigned short y)
{
	HalIoctl("display",DISPLAY_HAL_IO_SET_CURSOR, y * SCREEN_WIDTH + x);
}

PUBLIC void ConsoleSetColor(unsigned char color)
{
	HalIoctl("display",DISPLAY_HAL_IO_SET_COLOR, color);
}

PUBLIC void ConsoleReadGotoXY(unsigned short x, unsigned short y)
{
	HalIoctl("display",DISPLAY_HAL_IO_RDCURSOR, y * SCREEN_WIDTH + x);
}

PUBLIC int SysLog(char *buf)
{
	ConsoleWrite((const char *)buf, strlen(buf));
	return 0;
}