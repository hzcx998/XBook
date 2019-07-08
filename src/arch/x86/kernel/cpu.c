/*
 * file:		arch/x86/kernel/cpu.c
 * auther:		Jason Hu
 * time:		2019/6/27
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/debug.h>
#include <book/hal.h>
#include <hal/char/cpu.h>

/*
 * CpuInit - 初始化cpu并读取信息
 */
PUBLIC void CpuInit()
{
	PART_START("Cpu");

	//打开hal
	HalOpen("cpu");

	char buf[64];
	HalIoctl("cpu", CPU_HAL_IO_STEER, CPU_HAL_VENDOR);
	HalRead("cpu", (unsigned char *)buf, 64);
	printk("\n |- vendor:%s \n", buf);

	HalIoctl("cpu", CPU_HAL_IO_STEER, CPU_HAL_BRAND);
	HalRead("cpu", (unsigned char *)buf, 64);
	printk(" |- brand:%s \n", buf);
	
	#ifdef CONFIG_CPU_DEBUG
	
	HalIoctl("cpu", CPU_HAL_IO_STEER, CPU_HAL_FAMILY);
	HalRead("cpu", (unsigned char *)buf, 64);
	printk(" |- family:%s\n", buf);
	
	HalIoctl("cpu", CPU_HAL_IO_STEER, CPU_HAL_MODEL);
	HalRead("cpu", (unsigned char *)buf, 64);
	printk(" |- model:%s\n", buf);
	
	HalIoctl("cpu", CPU_HAL_IO_STEER, CPU_HAL_TYPE);
	HalRead("cpu", (unsigned char *)buf, 4);
	printk(" |- type:%x\n", *(int *)&buf);

	HalIoctl("cpu", CPU_HAL_IO_STEER, CPU_HAL_STEEPING);
	HalRead("cpu", (unsigned char *)buf, 4);
	printk(" |- steeping:%d\n", *(int *)&buf);
	
	HalIoctl("cpu", CPU_HAL_IO_STEER, CPU_HAL_MAX_CPUID);
	HalRead("cpu", (unsigned char *)buf, 4);
	printk(" |- cpuid:%x\n", *(int *)&buf);
	
	HalIoctl("cpu", CPU_HAL_IO_STEER, CPU_HAL_MAX_CPUID_EXT);
	HalRead("cpu", (unsigned char *)buf, 4);
	printk(" |- cpuid externed:%x\n", *(int *)&buf);
	#endif
	PART_END();
}


