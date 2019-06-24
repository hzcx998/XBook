/*
 * file:		arch/x86/include/kernel/gate.h
 * auther:		Jason Hu
 * time:		2019/6/23
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _ARCH_INTERRUPT_H
#define _ARCH_INTERRUPT_H

#include <share/stdint.h>

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

enum InterruptStatus InterruptGetStatus(void);
enum InterruptStatus InterruptSetStatus(enum InterruptStatus status);
enum InterruptStatus InterruptEnable(void);
enum InterruptStatus InterruptDisable(void);

void InterruptRegisterHandler(unsigned char interrupt, intr_handler_t function);
void InterruptCancelHandler(unsigned char interrupt);

void EnableIRQ(unsigned char irq);
void DisableIRQ(unsigned char irq);

#endif	/*_ARCH_INTERRUPT_H*/