/*
 * file:		arch/x86/include/kernel/cpu.h
 * auther:		Jason Hu
 * time:		2019/6/27
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _ARCH_CPU_H
#define _ARCH_CPU_H

#include <share/types.h>
#include <share/stdint.h>

//#define CONFIG_CPU_DEBUG

/* 内核栈 */
#define KERNEL_STATCK_TOP		0x8009f000
#define KERNEL_STATCK_BOTTOM	(KERNEL_STATCK_TOP - PAGE_SIZE)

/* 任务状态 */
typedef struct Tss 
{
	uint32_t backlink;
	uint32_t esp0;	//we will use esp 
	uint32_t ss0;	//stack segment
	uint32_t esp1;
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldtr;
	uint32_t trap;
	uint32_t iobase;
} Tss_t;

EXTERN Tss_t tss;

PUBLIC void InitCpu();

PUBLIC void InitTss();
PUBLIC Tss_t *GetTss();

#endif	/*_ARCH_CPU_H*/