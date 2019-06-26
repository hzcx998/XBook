/*
 * file:		arch/x86/include/kernel/gate.h
 * auther:		Jason Hu
 * time:		2019/6/23
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _ARCH_INTERRUPT_H
#define _ARCH_INTERRUPT_H

#include <share/stdint.h>
#include <share/types.h>

/* 中断分配管理 */

/* IRQ */
#define	IRQ0_CLOCK          0   // 时钟
#define	IRQ1_KEYBOARD       1   // 键盘
#define	IRQ2_CONNECT        2   // 连接从片
#define	IRQ3_SERIAL2        3   // 串口2
#define	IRQ4_SERIAL1        4   // 串口1
#define	IRQ5_PARALLEL2      5   // 并口2
#define	IRQ6_FLOPPY         6   // 软盘
#define	IRQ5_PARALLEL1      7   // 并口1

#define	IRQ8_RTCLOCK        8   // 实时时钟（real-time clock）
#define	IRQ9_REDIRECT       9   // 重定向的IRQ2
#define	IRQ10_RESERVE       10  // 保留
#define	IRQ11_RESERVE       11  // 保留
#define	IRQ12_MOUSE         12  // PS/2鼠标
#define	IRQ13_FPU           13  // FPU异常
#define	IRQ14_HARDDISK      14  // 硬盘
#define	IRQ15_RESERVE       15  // 保留

//EFLAGS
#define	EFLAGS_MBS (1<<1)
#define	EFLAGS_IF_1 (1<<9)
#define	EFLAGS_IF_0 0
#define	EFLAGS_IOPL_3 (3<<12)
#define	EFLAGS_IOPL_1 (1<<12)
#define	EFLAGS_IOPL_0 (0<<12)

/* IF 位是在 eflags寄存器的低9位 */
#define EFLAGS_IF (EFLAGS_IF_1 << 9)

//中断处理函数的类型
typedef void* intr_handler_t;

/* 定义中断的两种状态:
 * INTERRUPT_OFF值为0,表示关中断,
 * INTERRUPT_ON值为1,表示开中断 */
enum InterruptStatus {		 // 中断状态
    INTERRUPT_OFF,			 // 中断关闭
    INTERRUPT_ON		         // 中断打开
};

enum InterruptStatus InterruptGetStatus();
enum InterruptStatus InterruptSetStatus(enum InterruptStatus status);
enum InterruptStatus InterruptEnable();
enum InterruptStatus InterruptDisable();

PUBLIC void InterruptRegisterHandler(unsigned char interrupt, intr_handler_t function);
PUBLIC void InterruptCancelHandler(unsigned char interrupt);

PUBLIC void IrqRegisterHandler(unsigned char irq, intr_handler_t function);
PUBLIC void IrqCancelHandler(unsigned char irq);

PUBLIC void EnableIRQ(unsigned char irq);
PUBLIC void DisableIRQ(unsigned char irq);

#endif	/*_ARCH_INTERRUPT_H*/