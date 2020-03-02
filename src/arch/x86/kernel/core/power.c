/*
 * file:		arch/x86/kernel/core/cmos.h
 * auther:		Jason Hu
 * time:		2019/9/6
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <kernel/x86.h>
#include <kernel/cmos.h>
#include <kernel/gate.h>

#include <lib/types.h>
#include <book/power.h>

void ArchDoReboot(int type)
{
	/*不能再switch语句中申请变量，所以放到外面来*/
	uint8_t rebootCode;
	uint8_t cf9;
	struct GateDescriptor *idt;
	int i;
	switch(type){
		case REBOOT_KBD:
			/*
			8042 键盘控制器的错误使用方法
			向 8042 键盘控制器（端口 0x64）写入 0xFE. 
			这会触发 CPU 的重置信号，并重启计算机。
			*/
			Out8(0x64, 0xfe);
			break;
		case REBOOT_CF9:
			/*
			向 PCI 的 0xCF9 端口发送重置信号
			Intel 的 ICH/PCH 南桥芯片同时负责部分电源工作。向 PCI 的 0xCF9 端口
			发送重置信号，可以要求南桥芯片重启计算机。
			*/
			rebootCode = 0x06;	/*热重启，冷重启0x0e*/
			cf9 = In8(0xcf9) & ~rebootCode;	
			Out8(0xcf9, cf9 | 2);	/*Request hard reset*/
			Out8(0xcf9, cf9 | rebootCode);	/*Actually do the reset*/
			break;
		case REBOOT_TRIPLE:
			EnableInterrupt();
			//清空中断描述符表
			idt = (struct GateDescriptor *)IDT_VADDR;
			for (i = 0; i <= IDT_LIMIT / 8; i++) {
				SetGateDescriptor(idt + i, 0, 0, 0, 0);
			}

			LoadIDTR(IDT_LIMIT, IDT_VADDR);
			//触发中断
			__asm__ __volatile__("ud2");
			/* 此时发生中断，由于 IDT 为空，无论是中断的异常处理代码还是双重异常处理代码都不存在，触发了三重异常
			CPU 进入 SHUTDOWN 状态，主板重置 */
			break;
		default:
			break;
	}
}
