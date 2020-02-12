/*
 * file:		arch/x86/kernel/core/arch.c
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <kernel/arch.h>
#include <kernel/segment.h>
#include <kernel/gate.h>
#include <kernel/tss.h>
#include <mm/phymem.h>
#include <mm/bootmem.h>
#include <book/debug.h>
#include <book/hal.h>

/*
 * 功能: 平台架构初始化入口
 * 参数: 无
 * 返回: 成功返回0，失败返回-1
 * 说明: 初始化平台的所有信息，初始化完成后再跳入到内核执行
 */
int InitArch()
{	
	// 初始化硬件抽象层的环境
	InitHalEnvironment();

	// 初始化内核段描述符
	InitSegmentDescriptor();
	// 初始化内核门描述符
	InitGateDescriptor();
	
	// 初始化任务状态段
	InitTss();

	// 初始化物理内存管理
	InitPhysicMemory();
    
	return 0;
}
