/*
 * file:		arch/x86/kernel/core/tss.c
 * auther:		Jason Hu
 * time:		2020/2/9
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <kernel/tss.h>
#include <kernel/x86.h>
#include <kernel/segment.h>
#include <book/debug.h>
#include <lib/string.h>
#include <book/task.h>

/* tss对象 */
Tss_t tss;

PUBLIC void InitTss()
{

	memset(&tss, 0, sizeof(tss));
	// 内核的内核栈
	tss.esp0 = KERNEL_STATCK_TOP;
	// 内核栈选择子
	tss.ss0 = KERNEL_DATA_SEL;

	tss.iobase = sizeof(tss);
	// 加载tss register
	LoadTR(KERNEL_TSS_SEL);

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
