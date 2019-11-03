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
#include <book/memcache.h>
#include <user/conio.h>
#include <book/task.h>
#include <user/stdlib.h>
#include <user/mman.h>
#include <book/atomic.h>
#include <driver/keyboard.h>
#include <book/interrupt.h>
#include <book/device.h>
#include <driver/ide.h>
#include <book/vmarea.h>
#include <fs/partition.h>
#include <fs/interface.h>
#include <driver/ramdisk.h>
#include <book/block.h>
#include <fs/super_block.h>
#include <book/char.h>
#include <fs/flat.h>

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

	// 初始化内存缓存
	InitMemCaches();

	// 初始化内存区域
	InitVMArea();

	/* 初始化IRQ描述结构 */
	InitIrqDescription();

    /* 初始化软中断机制 */
    InitSoftirq();
	
	// 初始化多任务
	InitTasks();
	
	// 初始化多任务后，初始化工作队列
	InitWorkQueue();
	
	// 打开中断标志
	EnableInterrupt();
	
	//初始化时钟驱动
	InitClockDriver();
	
	/* 初始化ramdisk */
	//InitRamdiskDriver();

	/* 初始化字符设备 */	
	InitCharDevice();

	/* 初始化块设备 */	
	InitBlockDevice();

	InitFlatFileSystem();
	
	/* 初始化文件系统 */
	//InitFileSystem();
	
	/* 初始化文件系统 */
	//InitFileSystem();

	//Spin("bofs test");
	
	/* 加载init进程 */
	//InitFirstProcess("/bin/init", "init");
	//BlockDeviceTest();
	
	/* main thread 就是idle线程 */
	while (1) {
		
		/* 进程默认处于阻塞状态，如果被唤醒就会执行后面的操作，
		知道再次被阻塞 */
		TaskBlock(TASK_BLOCKED);
		/* 打开中断 */
		EnableInterrupt();
		/* 执行cpu停机 */
		CpuHlt();
	};
	PART_END();
	return 0;
}
