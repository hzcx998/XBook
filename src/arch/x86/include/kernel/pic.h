/*
 * file:		arch/x86/include/kernel/pic.h
 * auther:		Jason Hu
 * time:		2019/6/23
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _X86_PIC_H
#define _X86_PIC_H

#include <lib/stdint.h>

/*
各个端口
*/
#define PIC0_ICW1		0x0020
 //主
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
 //从
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1


#define INT_M_CTL		PIC0_OCW2	// I/O port for interrupt controller         <Master>
#define INT_M_CTLMASK   PIC0_IMR	// setting bits in this port disables ints   <Master>
#define INT_S_CTL	    PIC1_OCW2	// I/O port for second interrupt controller  <Slave>
#define INT_S_CTLMASK	PIC1_IMR	// setting bits in this port disables ints   <Slave>

#define PIC_EIO		    0X20

void InitPic();
/*
void PicEnable(unsigned int irq);
void PicDisable(unsigned int irq);
unsigned int PicInstall(unsigned int irq, void * arg);
void PicUninstall(unsigned int irq);
void PicAck(unsigned int irq);
*/
#endif	/* _X86_PIC_H */