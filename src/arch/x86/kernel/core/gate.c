/*
 * file:		arch/x86/kernel/core/gate.c
 * auther:		Jason Hu
 * time:		2019/6/23
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <kernel/x86.h>
#include <kernel/gate.h>
#include <kernel/segment.h>
#include <kernel/pic.h>
#include <lib/types.h>
#include <lib/stddef.h>
#include <book/debug.h>
#include <book/task.h>
#include <book/signal.h>

#define _DEBUG_GATE

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

EXTERN void ExceptionEntry0x00();
EXTERN void ExceptionEntry0x01();
EXTERN void ExceptionEntry0x02();
EXTERN void ExceptionEntry0x03();
EXTERN void ExceptionEntry0x04();
EXTERN void ExceptionEntry0x05();
EXTERN void ExceptionEntry0x06();
EXTERN void ExceptionEntry0x07();
EXTERN void ExceptionEntry0x08();
EXTERN void ExceptionEntry0x09();
EXTERN void ExceptionEntry0x0a();
EXTERN void ExceptionEntry0x0b();
EXTERN void ExceptionEntry0x0c();
EXTERN void ExceptionEntry0x0d();
EXTERN void ExceptionEntry0x0e();
EXTERN void ExceptionEntry0x0f();
EXTERN void ExceptionEntry0x10();
EXTERN void ExceptionEntry0x11();
EXTERN void ExceptionEntry0x12();
EXTERN void ExceptionEntry0x13();
EXTERN void ExceptionEntry0x14();
EXTERN void ExceptionEntry0x15();
EXTERN void ExceptionEntry0x16();
EXTERN void ExceptionEntry0x17();
EXTERN void ExceptionEntry0x18();
EXTERN void ExceptionEntry0x19();
EXTERN void ExceptionEntry0x1a();
EXTERN void ExceptionEntry0x1b();
EXTERN void ExceptionEntry0x1c();
EXTERN void ExceptionEntry0x1d();
EXTERN void ExceptionEntry0x1e();
EXTERN void ExceptionEntry0x1f();

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
	SetGateDescriptor(&idt[0x00], ExceptionEntry0x00, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x01], ExceptionEntry0x01, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x02], ExceptionEntry0x02, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x03], ExceptionEntry0x03, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x04], ExceptionEntry0x04, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x05], ExceptionEntry0x05, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x06], ExceptionEntry0x06, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x07], ExceptionEntry0x07, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x08], ExceptionEntry0x08, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x09], ExceptionEntry0x09, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x0a], ExceptionEntry0x0a, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x0b], ExceptionEntry0x0b, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x0c], ExceptionEntry0x0c, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x0d], ExceptionEntry0x0d, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x0e], ExceptionEntry0x0e, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x0f], ExceptionEntry0x0f, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x10], ExceptionEntry0x10, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x11], ExceptionEntry0x11, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x12], ExceptionEntry0x12, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x13], ExceptionEntry0x13, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x14], ExceptionEntry0x14, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x15], ExceptionEntry0x15, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x16], ExceptionEntry0x16, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x17], ExceptionEntry0x17, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x18], ExceptionEntry0x18, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x19], ExceptionEntry0x19, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x1a], ExceptionEntry0x1a, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x1b], ExceptionEntry0x1b, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x1c], ExceptionEntry0x1c, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x1d], ExceptionEntry0x1d, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x1e], ExceptionEntry0x1e, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x1f], ExceptionEntry0x1f, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	
	/*
	 IRQ的中断入口
	 */
	SetGateDescriptor(&idt[0x20], InterruptEntry0x20, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x21], InterruptEntry0x21, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x22], InterruptEntry0x22, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x23], InterruptEntry0x23, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x24], InterruptEntry0x24, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x25], InterruptEntry0x25, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x26], InterruptEntry0x26, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x27], InterruptEntry0x27, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x28], InterruptEntry0x28, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x29], InterruptEntry0x29, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x2a], InterruptEntry0x2a, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x2b], InterruptEntry0x2b, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x2c], InterruptEntry0x2c, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x2d], InterruptEntry0x2d, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x2e], InterruptEntry0x2e, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	SetGateDescriptor(&idt[0x2f], InterruptEntry0x2f, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL0); 
	
	/*
	 系统调用的中断入口
	 */
	SetGateDescriptor(&idt[SYSCALL_INTERRUPT_NR], SyscallHandler, KERNEL_CODE_SEL, DA_386IGate, DA_GATE_DPL3);
	 
	LoadIDTR(IDT_LIMIT, IDT_VADDR);

}

/* 
 * 通用的中断处理函数,一般用在异常出现时的处理 
 */
PUBLIC void IntrruptGeneralHandler(uint32_t esp) 
{
		// 中断栈
	struct TrapFrame *frame = (struct TrapFrame *)((uint32_t )esp);

	// 0x2f是从片8259A上的最后一个irq引脚，保留
	if (frame->vec_no == 0x27 || frame->vec_no == 0x2f) {	
      	return;		//IRQ7和IRQ15会产生伪中断(spurious interrupt),无须处理。
   	}
	
    /* 支持信号后的处理 */
    switch (frame->vec_no) {
    case EP_DIVIDE:
    case EP_DEVICE_NOT_AVAILABLE:
    case EP_COPROCESSOR_SEGMENT_OVERRUN:
    case EP_X87_FLOAT_POINT:
    case EP_SIMD_FLOAT_POINT:
#ifdef _DEBUG_GATE
        printk("send SIGFPE to %s!\n", interruptNameTable[frame->vec_no]);
#endif
        ForceSignal(SIGFPE, SysGetPid());
        return;
    case EP_OVERFLOW:
#ifdef _DEBUG_GATE
        printk("send SIGSTKFLT to %s!\n", interruptNameTable[frame->vec_no]);
#endif
        ForceSignal(SIGSTKFLT, SysGetPid());
        return;
    case EP_BOUND_RANGE:
    case EP_SEGMENT_NOT_PRESENT:
    case EP_STACK_FAULT:
#ifdef _DEBUG_GATE
        printk("send SIGSEGV to %s!\n", interruptNameTable[frame->vec_no]);
#endif
        ForceSignal(SIGSEGV, SysGetPid());
        return;
    case EP_INVALID_TSS:
    case EP_GENERAL_PROTECTION:
    case EP_ALIGNMENT_CHECK:
    case EP_MACHINE_CHECK:
#ifdef _DEBUG_GATE
        printk("send SIGBUS to %s!\n", interruptNameTable[frame->vec_no]);
#endif
        ForceSignal(SIGBUS, SysGetPid());
        return;
    case EP_INVALID_OPCODE:
#ifdef _DEBUG_GATE
        printk("send SIGILL to %s!\n", interruptNameTable[frame->vec_no]);
#endif
        ForceSignal(SIGILL, SysGetPid());
        return;
    case EP_DEBUG:
    case EP_BREAKPOINT:
#ifdef _DEBUG_GATE
        printk("send SIGTRAP to %s!\n", interruptNameTable[frame->vec_no]);
#endif
        ForceSignal(SIGTRAP, SysGetPid());
        return;
    case EP_PAGE_FAULT:
        /* 后面处理 */
        break;
    case EP_INTERRUPT:
    case EP_DOUBLE_FAULT:
#ifdef _DEBUG_GATE
        printk("send SIGTRAP to %s!\n", interruptNameTable[frame->vec_no]);
#endif
        ForceSignal(SIGABRT, SysGetPid());
        return;
    }
    
    /* 原始处理方式 */

	DebugColor(TEXT_RED);
    
	printk("! Exception messag start.\n");
	DebugColor(TEXT_GREEN);
	printk("name: %s \n", interruptNameTable[frame->vec_no]);

	printk("frame:\n");
	
	printk("vec: %x\n", 
			frame->vec_no);
	//Panic("expection");
	printk("edi: %x esi: %x ebp: %x esp: %x\n", 
			frame->edi, frame->esi, frame->ebp, frame->esp);
	printk("ebx: %x edx: %x ecx: %x eax: %x\n", 
			frame->ebx, frame->edx, frame->ecx, frame->eax);
	printk("gs: %x fs: %x es: %x ds: %x\n", 
			frame->gs, frame->fs, frame->es, frame->ds);
	printk("err: %x eip: %x cs: %x eflags: %x\n", 
			frame->errorCode, frame->eip, frame->cs, frame->eflags);
	printk("esp: %x ss: %x\n", 
			frame->esp, frame->ss);
	
	if(frame->errorCode != 0xFFFFFFFF){
		printk("Error code:%x\n", frame->errorCode);
		
		if(frame->errorCode&1){
			printk("    External Event: NMI,hard interruption,ect.\n");
		}else{
			printk("    Not External Event: inside.\n");
		}
		if(frame->errorCode&(1<<1)){
			printk("    IDT: selector in idt.\n");
		}else{
			printk("    IDT: selector in gdt or ldt.\n");
		}
		if(frame->errorCode&(1<<2)){
			printk("    TI: selector in ldt.\n");
		}else{
			printk("    TI: selector in gdt.\n");
		}
		printk("    Selector: idx %d\n", (frame->errorCode&0xfff8)>>3);
	}

    if (frame->vec_no == EP_PAGE_FAULT) {
        unsigned int pageFaultVaddr = 0; 
		pageFaultVaddr = ReadCR2();
		printk("page fault addr is: %x\n", pageFaultVaddr);
    }
	/*printk("    task %s %x kstack %x.\n", task->name, task, task->kstack);
	printk("    pgdir %x cr3 %x\n", task->pgdir, PageAddrV2P((uint32_t)task->pgdir));
	*/	

	DebugColor(TEXT_RED);
	printk("! Exception Messag done.\n");
  	// 能进入中断处理程序就表示已经处在关中断情况下,
  	// 不会出现调度进程的情况。故下面的死循环不会再被中断。
   	Panic("expection");
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
	interruptNameTable[15] = "Reserved"; //第15项是intel保留项，未使用
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
	descriptor->attributes	= attributes | (privilege << 5);
	descriptor->datacount   = 0;
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

PUBLIC unsigned long InterruptSave()
{
    unsigned long eflags = LoadEflags();
    DisableInterrupt();
    return eflags;
} 

PUBLIC void InterruptRestore(unsigned long eflags)
{
    StoreEflags(eflags);
} 

uint32_t vec_no;	 // kernel.S 宏VECTOR中push %1压入的中断号
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t espDummy;	 // 虽然pushad把esp也压入,但esp是不断变化的,所以会被popad忽略
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t errorCode;		 // errorCode会被压入在eip之后
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;

    /* 以下由cpu从低特权级进入高特权级时压入 */
    uint32_t esp;
    uint32_t ss;


PUBLIC void DumpTrapFrame(struct TrapFrame *frame)
{
#ifdef _DEBUG_GATE
    printk(PART_TIP "----Trap Frame----\n");
    printk(PART_TIP "vector:%d edi:%x esi:%x ebp:%x esp dummy:%x ebx:%x edx:%x ecx:%x eax:%x\n",
        frame->edi, frame->esi, frame->ebp, frame->espDummy, frame->ebx, frame->edx, frame->ecx, frame->eax);
    printk(PART_TIP "gs:%x fs:%x es:%x ds:%x error code:%x eip:%x cs:%x eflags:%x esp:%x ss:%x\n",
        frame->gs, frame->fs, frame->es, frame->ds, frame->errorCode, frame->eip, frame->cs, frame->eflags, frame->esp, frame->ss);
#endif 
}


PUBLIC void InitGateDescriptor()
{
	
	InitInterruptDescriptor();
	InitExpection();
	InitPic();

	LoadIDTR(IDT_LIMIT, IDT_VADDR);
	
}
