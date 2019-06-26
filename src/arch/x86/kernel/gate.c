/*
 * file:		arch/x86/kernel/gate.c
 * auther:		Jason Hu
 * time:		2019/6/23
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <x86.h>
#include <gate.h>
#include <segment.h>
#include <pic.h>

#include <share/types.h>
#include <share/stddef.h>
#include <book/debug.h>

/* 
 * Global Descriptor Table
 */
struct GateDescriptor *idt;

// 总共支持的中断数
#define MAX_IDT_NR (IDT_LIMIT/8)      

// 目前需要支持的中断数
#define MAX_INTERRUPT_NR 0x81

#define SYSCALL_INTERRUPT_NR 0x80

// 中断处理函数组成的数组
intr_handler_t interruptHandlerTable[MAX_INTERRUPT_NR];

// 用于保存异常的名字
char* interruptNameTable[MAX_INTERRUPT_NR];		     

// 汇编部分的中断处理函数的入口点组成的数组
EXTERN intr_handler_t interruptEntryTable[MAX_INTERRUPT_NR];	    // 声明引用定义在kernel.S中的中断处理函数入口数组

EXTERN void InterruptEntry0x00();
EXTERN void InterruptEntry0x01();
EXTERN void InterruptEntry0x02();
EXTERN void InterruptEntry0x03();
EXTERN void InterruptEntry0x04();
EXTERN void InterruptEntry0x05();
EXTERN void InterruptEntry0x06();
EXTERN void InterruptEntry0x07();
EXTERN void InterruptEntry0x08();
EXTERN void InterruptEntry0x09();
EXTERN void InterruptEntry0x0a();
EXTERN void InterruptEntry0x0b();
EXTERN void InterruptEntry0x0c();
EXTERN void InterruptEntry0x0d();
EXTERN void InterruptEntry0x0e();
EXTERN void InterruptEntry0x0f();
EXTERN void InterruptEntry0x10();
EXTERN void InterruptEntry0x11();
EXTERN void InterruptEntry0x12();
EXTERN void InterruptEntry0x13();
EXTERN void InterruptEntry0x14();
EXTERN void InterruptEntry0x15();
EXTERN void InterruptEntry0x16();
EXTERN void InterruptEntry0x17();
EXTERN void InterruptEntry0x18();
EXTERN void InterruptEntry0x19();
EXTERN void InterruptEntry0x1a();
EXTERN void InterruptEntry0x1b();
EXTERN void InterruptEntry0x1c();
EXTERN void InterruptEntry0x1d();
EXTERN void InterruptEntry0x1e();
EXTERN void InterruptEntry0x1f();

EXTERN void InterruptEntry0x20();
EXTERN void InterruptEntry0x21();
EXTERN void InterruptEntry0x22();
EXTERN void InterruptEntry0x23();
EXTERN void InterruptEntry0x24();
EXTERN void InterruptEntry0x25();
EXTERN void InterruptEntry0x26();
EXTERN void InterruptEntry0x27();
EXTERN void InterruptEntry0x28();
EXTERN void InterruptEntry0x29();
EXTERN void InterruptEntry0x2a();
EXTERN void InterruptEntry0x2b();
EXTERN void InterruptEntry0x2c();
EXTERN void InterruptEntry0x2d();
EXTERN void InterruptEntry0x2e();
EXTERN void InterruptEntry0x2f();

EXTERN void SyscallHandler();


PRIVATE void InitInterruptDescriptor()
{
	idt = (struct GateDescriptor *) IDT_VADDR;
	
	/*
	 将中断描述符表的内容设置成内核下的中断门
	 并把汇编部分的中断处理函数传入进去
	 */
	int i;
   	for (i = 0; i < MAX_IDT_NR; i++) {
      	SetGateDescriptor(&idt[i], 0, 0, 0, 0); 
   	}
	/*
	 异常的中断入口
	 */
	SetGateDescriptor(&idt[0x00], InterruptEntry0x00, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x01], InterruptEntry0x01, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x02], InterruptEntry0x02, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x03], InterruptEntry0x03, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x04], InterruptEntry0x04, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x05], InterruptEntry0x05, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x06], InterruptEntry0x06, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x07], InterruptEntry0x07, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x08], InterruptEntry0x08, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x09], InterruptEntry0x09, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x0a], InterruptEntry0x0a, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x0b], InterruptEntry0x0b, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x0c], InterruptEntry0x0c, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x0d], InterruptEntry0x0d, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x0e], InterruptEntry0x0e, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x0f], InterruptEntry0x0f, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x10], InterruptEntry0x10, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x11], InterruptEntry0x11, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x12], InterruptEntry0x12, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x13], InterruptEntry0x13, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x14], InterruptEntry0x14, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x15], InterruptEntry0x15, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x16], InterruptEntry0x16, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x17], InterruptEntry0x17, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x18], InterruptEntry0x18, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x19], InterruptEntry0x19, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x1a], InterruptEntry0x1a, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x1b], InterruptEntry0x1b, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x1c], InterruptEntry0x1c, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x1d], InterruptEntry0x1d, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x1e], InterruptEntry0x1e, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x1f], InterruptEntry0x1f, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	
	/*
	 IRQ的中断入口
	 */
	SetGateDescriptor(&idt[0x20], InterruptEntry0x20, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x21], InterruptEntry0x21, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x22], InterruptEntry0x22, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x23], InterruptEntry0x23, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x24], InterruptEntry0x24, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x25], InterruptEntry0x25, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x26], InterruptEntry0x26, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x27], InterruptEntry0x27, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x28], InterruptEntry0x28, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x29], InterruptEntry0x29, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x2a], InterruptEntry0x2a, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x2b], InterruptEntry0x2b, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x2c], InterruptEntry0x2c, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x2d], InterruptEntry0x2d, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x2e], InterruptEntry0x2e, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	SetGateDescriptor(&idt[0x2f], InterruptEntry0x2f, KERNEL_CODE_SEL, DA_386IGate, DA_DPL0); 
	
	/*
	 系统调用的中断入口
	 */
	SetGateDescriptor(&idt[SYSCALL_INTERRUPT_NR], SyscallHandler, KERNEL_CODE_SEL, DA_386IGate, DA_DPL3); 
}

/* 
 * 通用的中断处理函数,一般用在异常出现时的处理 
 */
PUBLIC void IntrruptGeneralHandler(unsigned char interrupt) 
{
	// 0x2f是从片8259A上的最后一个irq引脚，保留
	if (interrupt == 0x27 || interrupt == 0x2f) {	
      	return;		//IRQ7和IRQ15会产生伪中断(spurious interrupt),无须处理。
   	}
	
	ConsoleSetColor(TEXT_RED);
	ConsolePrint("! Exception messag start.\n");
	ConsoleSetColor(TEXT_GREEN);
	ConsolePrint("name: %s \n", interruptNameTable[interrupt]);

   	if (interrupt == 14) {	  // 若为Pagefault,将缺失的地址打印出来并悬停
      	unsigned int pageFaultVaddr = 0; 
		pageFaultVaddr = ReadCR2();

		ConsolePrint("page fault addr is: %x\n", pageFaultVaddr);
   	}
	ConsoleSetColor(TEXT_RED);
	ConsolePrint("! Exception Messag done.\n");
  	// 能进入中断处理程序就表示已经处在关中断情况下,
  	// 不会出现调度进程的情况。故下面的死循环不会再被中断。
   	while(1);
}

/* 完成一般中断处理函数注册及异常名称注册 */
PRIVATE void InitExpection(void)
{
	int i;
	//设置中断处理函数
   	for (i = 0; i < MAX_INTERRUPT_NR; i++) {

		// 默认为IntrruptGeneralHandler
      	interruptHandlerTable[i] = IntrruptGeneralHandler;		    
		
		// 以后会由RegisterHandler来注册具体处理函数。

		// 先统一赋值为unknown 
      	interruptNameTable[i] = "unknown";    
   	}
	interruptNameTable[0] = "#DE Divide Error";
	interruptNameTable[1] = "#DB Debug Exception";
	interruptNameTable[2] = "NMI Interrupt";
	interruptNameTable[3] = "#BP Breakpoint Exception";
	interruptNameTable[4] = "#OF Overflow Exception";
	interruptNameTable[5] = "#BR BOUND Range Exceeded Exception";
	interruptNameTable[6] = "#UD Invalid Opcode Exception";
	interruptNameTable[7] = "#NM Device Not Available Exception";
	interruptNameTable[8] = "#DF Double Fault Exception";
	interruptNameTable[9] = "Coprocessor Segment Overrun";
	interruptNameTable[10] = "#TS Invalid TSS Exception";
	interruptNameTable[11] = "#NP Segment Not Present";
	interruptNameTable[12] = "#SS Stack Fault Exception";
	interruptNameTable[13] = "#GP General Protection Exception";
	interruptNameTable[14] = "#PF Page-Fault Exception";
	// interruptNameTable[15] 第15项是intel保留项，未使用
	interruptNameTable[16] = "#MF x87 FPU Floating-Point Error";
	interruptNameTable[17] = "#AC Alignment Check Exception";
	interruptNameTable[18] = "#MC Machine-Check Exception";
	interruptNameTable[19] = "#XF SIMD Floating-Point Exception";
}

PUBLIC void SetGateDescriptor(struct GateDescriptor *descriptor, intr_handler_t offset, \
		unsigned int selector, unsigned int attributes, unsigned char privilege)
{
	descriptor->offsetLow   = (unsigned int)offset & 0xffff;
	descriptor->selector    = selector;
	descriptor->datacount   = (attributes >> 8) & 0xff;
	descriptor->attributes	= attributes | (privilege << 5);
	descriptor->offsetHigh  = ((unsigned int)offset >> 16) & 0xffff;

}

/* 
 * 在中断处理程序数组第vector_no个元素中注册安装中断处理程序function
 */
PUBLIC void InterruptRegisterHandler(unsigned char interrupt, intr_handler_t function) 
{
	//把函数写入到中断处理程序表
   	interruptHandlerTable[interrupt] = function; 
}

PUBLIC void InterruptCancelHandler(unsigned char interrupt)
{
	//把默认函数写入到中断处理程序表
   	interruptHandlerTable[interrupt] = IntrruptGeneralHandler; 
}

/* 
 * 注册IRQ中断
 */
PUBLIC void IrqRegisterHandler(unsigned char irq, intr_handler_t function) 
{
	/* 如果是不正确的irq号就退出 */
	if (irq < IRQ0_CLOCK || irq > IRQ15_RESERVE) {
		return;
	}
	//把函数写入到中断处理程序表
   	interruptHandlerTable[IDT_IRQ_START + irq] = function; 
}

/* 
 * 取消IRQ中断
 */
PUBLIC void IrqCancelHandler(unsigned char irq)
{
	/* 如果是不正确的irq号就退出 */
	if (irq < IRQ0_CLOCK || irq > IRQ15_RESERVE) {
		return;
	}
	//把默认函数写入到中断处理程序表
   	interruptHandlerTable[IDT_IRQ_START + irq] = IntrruptGeneralHandler; 
}

/*
 获取中断状态并打开中断
 */
PUBLIC enum InterruptStatus InterruptEnable()
{
	enum InterruptStatus oldStatus;
	//如果获取的状态是中断已经打开
   	if (InterruptGetStatus() == INTERRUPT_ON) {
		//保存当前中断状态并返回
      	oldStatus = INTERRUPT_ON;
      	return oldStatus;
   	} else {
		//中断是关闭的，需要打开中断
      	oldStatus = INTERRUPT_OFF;

		// 开中断,调用汇编打开中断函数，sti指令将IF位置1
      	EnableInterrupt();
      	return oldStatus;
   	}
}

/*
 获取中断状态并设置对应的中断状态
 */
PUBLIC enum InterruptStatus InterruptDisable()
{
	enum InterruptStatus oldStatus;
   	if (InterruptGetStatus() == INTERRUPT_ON) {
      	oldStatus = INTERRUPT_ON;

		// 关中断,调用汇编打开中断函数，cli指令将IF位置0
		DisableInterrupt();
      	return oldStatus;
   	} else {
      	oldStatus = INTERRUPT_OFF;
      	return oldStatus;
   	}
}

/* 
 将中断状态设置为status 
 */
PUBLIC enum InterruptStatus InterruptSetStatus(enum InterruptStatus status) {
   return status & INTERRUPT_ON ? InterruptEnable() : InterruptDisable();
}

/* 
 通过判断eflags中的IF位，获取当前中断状态 
 */
PUBLIC enum InterruptStatus InterruptGetStatus() 
{
   flags_t eflags = 0; 
   eflags = LoadEflags();
   return (eflags & EFLAGS_IF) ? INTERRUPT_ON : INTERRUPT_OFF;
}

PUBLIC void InitGateDescriptor()
{
	PART_START("Gate descriptor");

	InitInterruptDescriptor();
	InitExpection();
	InitPic();

	LoadIDTR(IDT_LIMIT, IDT_VADDR);
	
	PART_END();
}


