/*
 * file:		arch/x86/include/kernel/gate.h
 * auther:		Jason Hu
 * time:		2019/6/23
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _X86_GATE_H
#define _X86_GATE_H

#include <kernel/const.h>
#include <kernel/interrupt.h>
#include <lib/stdint.h>

/* IDT 的虚拟地址 */
#define IDT_VADDR		0x80200800
#define IDT_LIMIT		0x000007ff

#define	DA_TaskGate		0x85	/* 任务门类型值				*/
#define	DA_386CGate		0x8C	/* 386 调用门类型值			*/
#define	DA_386IGate		0x8E	/* 386 中断门类型值			*/
#define	DA_386TGate		0x8F	/* 386 陷阱门类型值			*/

#define DA_GATE_DPL0 0
#define DA_GATE_DPL1 1
#define DA_GATE_DPL2 2
#define DA_GATE_DPL3 3

enum {
    EP_DIVIDE = 0,                          /* 除法错误：DIV和IDIV指令 */
    EP_DEBUG,                               /* 调试异常：任何代码和数据的访问 */
    EP_INTERRUPT,                           /* 非屏蔽中断：非屏蔽外部中断 */
    EP_BREAKPOINT,                          /* 调试断点：指令INT3 */
    EP_OVERFLOW,                            /* 溢出：指令INTO */
    EP_BOUND_RANGE,                         /* 越界：指令BOUND */
    EP_INVALID_OPCODE,                      /* 无效（未定义）的操作码：指令UD2或者无效指令 */
    EP_DEVICE_NOT_AVAILABLE,                /* 设备不可用（无数学处理器）：浮点或WAIT/FWAIT指令 */
    EP_DOUBLE_FAULT,                        /* 双重错误：所有能产生异常或NMI或INTR的指令 */
    EP_COPROCESSOR_SEGMENT_OVERRUN,         /* 协助处理器段越界：浮点指令（386之后的IA32处理器不再产生此种异常） */
    EP_INVALID_TSS,                         /* 无效TSS：任务切换或访问TSS时 */
    EP_SEGMENT_NOT_PRESENT,                 /* 段不存在：加载段寄存器或访问系统段时 */
    EP_STACK_FAULT,                         /* 堆栈段错误：堆栈操作或加载SS时 */
    EP_GENERAL_PROTECTION,                  /* 常规保护错误：内存或其它保护检验 */
    EP_PAGE_FAULT,                          /* 页故障：内存访问 */
    EP_RESERVED,                            /* INTEL保留，未使用 */
    EP_X87_FLOAT_POINT,                     /* X87FPU浮点错（数学错）：X87FPU浮点指令或WAIT/FWAIIT指令 */
    EP_ALIGNMENT_CHECK,                     /* 对齐检验：内存中的数据访问（486开始支持） */
    EP_MACHINE_CHECK,                       /* Machine Check：错误码（如果有的话）和源依赖于具体模式（奔腾CPU开始支持） */
    EP_SIMD_FLOAT_POINT,                    /* SIMD浮点异常：SSE和SSE2浮点指令（奔腾III开始支持） */
};

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

#endif	/*_X86_SEGMENT_H*/