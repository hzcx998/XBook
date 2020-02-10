/*
 * file:		kernel/device/cpu.c
 * auther:		Jason Hu
 * time:		2019/6/27
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/cpu.h>
#include <book/debug.h>
#include <book/hal.h> 

/* tss对象 */
Tss_t tss;

/*
 * CpuInit - 初始化cpu并读取信息
 */
PUBLIC void InitCpu()
{
	PART_START("Cpu");

	//打开hal
	HalOpen("cpu");

	char buf[64];
	HalIoctl("cpu", CPU_HAL_IO_STEER, CPU_HAL_VENDOR);
	HalRead("cpu", (unsigned char *)buf, 64);
	printk("\n" PART_TIP "vendor:%s", buf);

	HalIoctl("cpu", CPU_HAL_IO_STEER, CPU_HAL_BRAND);
	HalRead("cpu", (unsigned char *)buf, 64);
	printk(PART_TIP " brand:%s \n", buf);
	
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
