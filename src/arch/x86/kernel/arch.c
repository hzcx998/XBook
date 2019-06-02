/*
 * file:		arch/x86/kernel/arch.c
 * auther:	Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <arch.h>

/*
 * 功能: 平台架构初始化入口
 * 参数: 无
 * 返回: 成功返回0，失败返回-1
 * 说明: 初始化平台的所有信息，初始化完成后再跳入到内核执行
 */
int init_arch()
{
	int *pdt = (int *)0x80201000;

	pdt[0] = 0;


	return 0;
}

