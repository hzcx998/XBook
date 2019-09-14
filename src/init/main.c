/*
 * file:		init/main.c
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/hal.h>
#include <book/debug.h>
#include <driver/clock.h>
#include <share/string.h>
#include <book/slab.h>
#include <book/vmarea.h>
#include <user/conio.h>
#include <book/task.h>
#include <user/stdlib.h>
#include <user/mman.h>
#include <book/vfs.h>
#include <book/atomic.h>
#include <driver/keyboard.h>
#include <book/interrupt.h>
#include <book/deviceio.h>
#include <driver/ide.h>
#include <book/vmalloc.h>
#include <fs/partition.h>

/*
 * 功能: 内核的主函数
 * 参数: 无
 * 返回: 不返回，只是假装返回
 * 说明: 引导和加载完成后，就会跳到这里
 */
int main()
{
	PART_START("Kernel main");
	
	//初始化硬件抽象层
	InitHalKernel();

	// 初始化slab缓冲区
	InitSlabCacheManagement();

	// 初始化虚拟内存区域
	//InitVirtualMemoryArea();

	InitVM_Area();

	/* 初始化IRQ描述结构 */
	InitIrqDescription();

    /* 初始化软中断机制 */
    InitSoftirq();
	
	// 初始化多任务
	InitTasks();
	
	// 初始化多任务后，初始化工作队列
	InitWorkQueue();
	
	// 打开中断标志
	InterruptEnable();
	
	/* 初始化设备I/O */
	InitDeviceIO();
	
	//初始化时钟驱动
	InitClock();

	/* 初始化键盘驱动 */
	InitKeyboardDriver();
	
	/* 初始化IDE硬盘驱动 */
	InitIDE_Driver();
	
	/* 初始化磁盘分区 */
	InitDiskPartiton();
	
	BOFS_Test();
	Spin("bofs test");
	/* 初始化虚拟文件系统 */
	InitVFS();
	
	/* 加载init进程 */
	InitFirstProcess("init", "init");

	int key = 0;
	/* main thread 就是idle线程 */
	while (1) {
		key = DeviceGetc(DEVICE_KEYBOARD);
		if (key != KEYCODE_NONE)
			printk("%c", key); 

		/* 进程默认处于阻塞状态，如果被唤醒就会执行后面的操作，
		知道再次被阻塞 */
		//TaskBlock(TASK_BLOCKED);
		/* 打开中断 */
		//EnableInterrupt();
		/* 执行cpu停机 */
		//CpuHlt();
	};
	PART_END();
	return 0;
}
