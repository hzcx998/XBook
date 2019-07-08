/*
 * file:		init/main.c
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/hal.h>
#include <book/debug.h>
#include <driver/clock.h>
#include <share/string.h>

/*
 * 功能: 内核的主函数
 * 参数: 无
 * 返回: 不返回，只是假装返回
 * 说明: 引导和加载完成后，就会跳到这里
 */
int main()
{
	PART_START("Kernel main");

	//初始化硬件抽象层
	InitHalKernel();

	//初始化时钟驱动
	ClockInit();

	

	
	PART_END();
	return 0;
}
