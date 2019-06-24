/*
 * file:		init/main.c
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/hal.h>
#include <hal/hal.h>
#include <book/debug.h>

/*
 * 功能: 内核的主函数
 * 参数: 无
 * 返回: 不返回，只是假装返回
 * 说明: 引导和加载完成后，就会跳到这里
 */
int main()
{
	printk("> main start.\n");
	//初始化硬件抽象层
	InitHal();
	




	printk("< main done.\n");
	return 0;
}
