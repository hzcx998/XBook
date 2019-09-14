/*
 * file:		hal/block/ram.c
 * auther:		Jason Hu
 * time:		2019/7/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/debug.h>
#include <share/stddef.h>
#include <hal/block/ram.h>
#include <book/arch.h>
#include <share/string.h>

PRIVATE struct RamPrivate {
	size_t memorySize;		//内存大小
}ramPrivate;

PRIVATE void RamHalIoctl(unsigned int cmd, unsigned int param)
{
	//根据类型设置不同的值
	switch (cmd)
	{
	case RAM_HAL_IO_MEMSIZE:
		*((unsigned int *)param) = ramPrivate.memorySize;
		break;
	default:
		break;
	}
}

PUBLIC void RamHalInit()
{
	PART_START("Ram hal");

	ramPrivate.memorySize = InitArds();
	printk(" |- ram memory size:%x\n", ramPrivate.memorySize);
	PART_END();
}

/* hal操作函数 */
PRIVATE struct HalOperate halOperate = {
	.Init = &RamHalInit,
	.Ioctl = &RamHalIoctl,
};

/* hal对象，需要把它导入到hal系统中 */
PUBLIC HAL(halOfRam, halOperate, "ram");
