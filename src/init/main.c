/*
 * file:		init/main.c
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/hal.h>
#include <book/debug.h>
#include <book/task.h>
#include <book/interrupt.h>
#include <book/device.h>
#include <book/cpu.h>
#include <book/fs.h>
#include <book/mmu.h>
#include <book/power.h>
#include <net/network.h>
#include <pci/pci.h>
#include <clock/clock.h>
#include <block/block.h>
#include <char/char.h>
#include <sound/sound.h>
#include <input/input.h>
#include <video/video.h>

/*
 * 功能: 内核的主函数
 * 参数: 无
 * 返回: 不返回，只是假装返回
 * 说明: 引导和加载完成后，就会跳到这里
 */
PUBLIC int main()
{
    /* 初始化CPU信息 */
    InitCpu();

    /* 初始化平台总线 */
	InitPci();

    /* 初始化内存管理单元 */
    InitMMU();

	/* 初始化IRQ描述结构 */
	InitIrqDescription();

    /* 初始化软中断机制 */
    InitSoftirq();	

	/* 初始化多任务 */
	InitTasks();

	/* 初始化多任务后，初始化工作队列 */
	InitWorkQueue();
	
	/* 打开中断标志 */
	EnableInterrupt();
	
	/* 初始化时钟系统 */
	InitClockSystem();
	
	/* 初始化字符设备 */	
	InitCharDevice();
    
	/* 初始化块设备 */	
	InitBlockDevice();
    
    /* 初始化网络 */
    InitNetworkDevice();

    /* 初始化输入系统 */
    InitInputSystem();

    /* 初始化音频系统 */
    InitSoundSystem();
    
    /* 初始化视频系统 */
    InitVideoSystem();
    /* 初始化文件系统 */
    InitFileSystem();

    //SysReboot(REBOOT_KBD);

    /* 执行最后的初始化设置，进入群雄逐鹿的场面 */
	InitUserProcess();
	/* main thread 就是idle线程 */
	while (1) {
		
		/* 进程默认处于阻塞状态，如果被唤醒就会执行后面的操作，
		知道再次被阻塞 */
		//TaskBlock(TASK_BLOCKED);
		/* 打开中断 */
		EnableInterrupt();
		/* 执行cpu停机 */
		CpuHlt();
	};
	return 0;
}
