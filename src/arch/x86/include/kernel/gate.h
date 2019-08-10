/*
 * file:		arch/x86/include/kernel/gate.h
 * auther:		Jason Hu
 * time:		2019/6/23
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _ARCH_GATE_H
#define _ARCH_GATE_H

#include <const.h>
#include <share/stdint.h>

#include <interrupt.h>

/* IDT 的虚拟地址 */
#define IDT_VADDR		0xc0200800
#define IDT_LIMIT		0x000007ff

#define	DA_TaskGate		0x85	/* 任务门类型值				*/
#define	DA_386CGate		0x8C	/* 386 调用门类型值			*/
#define	DA_386IGate		0x8E	/* 386 中断门类型值			*/
#define	DA_386TGate		0x8F	/* 386 陷阱门类型值			*/

#define DA_GATE_DPL0 0
#define DA_GATE_DPL1 1
#define DA_GATE_DPL2 2
#define DA_GATE_DPL3 3



/* IRQ中断在idt中的起始位置 */
#define IDT_IRQ_START	0X20

/*
门描述符结构
*/
struct GateDescriptor {
	unsigned short offsetLow, selector;
	unsigned char datacount;
	unsigned char attributes;		/* P(1) DPL(2) DT(1) TYPE(4) */
	unsigned short offsetHigh;
};

//初始化门描述符
PUBLIC void InitGateDescriptor();

//设置门描述符
PUBLIC void SetGateDescriptor(struct GateDescriptor *descriptor, intr_handler_t offset, \
		unsigned int selector, unsigned int attributes, unsigned char privilege);

#endif	/*_ARCH_SEGMENT_H*/