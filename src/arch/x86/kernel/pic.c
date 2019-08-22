/*
 * file:		arch/x86/kernel/pic.c
 * auther:		Jason Hu
 * time:		2019/6/23
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <pic.h>
#include <x86.h>
#include <interrupt.h>
#include <book/interrupt.h>

/**
 * InitPic - 初始化pic
 */
PUBLIC void InitPic(void)
{
	Out8(PIC0_IMR,  0xff  );
	Out8(PIC1_IMR,  0xff  );

	Out8(PIC0_ICW1, 0x11  );
	Out8(PIC0_ICW2, 0x20  );
	Out8(PIC0_ICW3, 1 << 2);
	Out8(PIC0_ICW4, 0x01  );

	Out8(PIC1_ICW1, 0x11  );
	Out8(PIC1_ICW2, 0x28  );
	Out8(PIC1_ICW3, 2     );
	Out8(PIC1_ICW4, 0x01  );

	//屏蔽所有中断
	Out8(PIC0_IMR,  0xff  );
	Out8(PIC1_IMR,  0xff  );
	
	return;
}

/**
 * PicEnable -  PIC中断打开irq
 * irq: 中断irq号
 */ 
PRIVATE void PicEnable(unsigned int irq)
{
	/* 如果是位于从片上，就要打开IRQ2_CONNECT */
	if (irq >= 8)
		EnableIRQ(IRQ2_CONNECT);

	EnableIRQ(irq);
}

/**
 * PicDisable -  PIC中断关闭irq
 * irq: 中断irq号
 */ 
PRIVATE void PicDisable(unsigned int irq)
{
	DisableIRQ(irq);
}

/**
 * PicInstall - PIC安装一个中断
 * irq: 中断irq号
 * arg: 安装传入的参数，一般是中断处理函数的地址
 */
PRIVATE unsigned int PicInstall(unsigned int irq, void * arg)
{
	IrqRegisterHandler((char )irq, (intr_handler_t)arg);
	return 1;
}

/**
 * PicInstall - PIC安装一个中断
 * irq: 中断irq号
 * arg: 安装传入的参数，一般是中断处理函数的地址
 */
PRIVATE void PicUninstall(unsigned int irq)
{
	IrqCancelHandler((char )irq);
}

/**
 * PicInstall - PIC安装一个中断
 * irq: 中断irq号
 * arg: 安装传入的参数，一般是中断处理函数的地址
 */
PRIVATE void PicAck(unsigned int irq)
{
	/* 从芯片 */
	if (irq >= 8) {
		Out8(INT_S_CTL,  PIC_EIO);
	}
	/* 主芯片 */
	Out8(INT_M_CTL,  PIC_EIO);
}

/* pic硬件中断控制方法 */
struct HardwareIntController picHardwareIntContorller = {
    .install = PicInstall, 
	.uninstall = PicUninstall,
	.enable = PicEnable,
	.disable = PicDisable,
	.ack = PicAck,
};

/**
 * DoIRQ - 处理irq中断
 * @frame: 获取的中断栈
 * 
 * 在保存环境之后，就执行这个操作，尝试执行中断
 */
PUBLIC int DoIRQ(struct TrapFrame *frame)
{
	unsigned int irq;
	/* 中断向量号减去异常占用的向量号，就是IRQ号 */
	irq = frame->vec_no - 0x20;

	/* 处理具体的中断 */
	if (!HandleIRQ(irq, frame)) {
		return -1;
	}
	return 0;
}