/*
 * file:		arch/x86/kernel/cpu.c
 * auther:		Jason Hu
 * time:		2019/6/27
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <cpu.h>
#include <x86.h>
#include <segment.h>
#include <book/debug.h>
#include <book/hal.h>
#include <share/string.h>
#include <book/task.h>


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

PUBLIC void InitTss()
{
	PART_START("Tss");

	memset(&tss, 0, sizeof(tss));
	// 内核的内核栈
	tss.esp0 = KERNEL_STATCK_TOP;
	// 内核栈选择子
	tss.ss0 = KERNEL_DATA_SEL;

	tss.iobase = sizeof(tss);
	// 加载tss register
	LoadTR(KERNEL_TSS_SEL);

	PART_END();
}

PUBLIC Tss_t *GetTss()
{
	return &tss;
}

PUBLIC void UpdateTssInfo(struct Task *task)
{
	// 更新tss.esp0的值为任务的内核栈顶
	tss.esp0 = (unsigned int)((uint32_t)task + PAGE_SIZE);
	// printk("task %s update tss esp0\n", task->name);
}
