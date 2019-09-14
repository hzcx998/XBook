/*
 * file:		driver/ide.c
 * auther:		Jason Hu
 * time:		2019/8/26
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <book/arch.h>
#include <book/config.h>
#include <book/debug.h>
#include <book/interrupt.h>
#include <driver/ide.h>
#include <book/ioqueue.h>
#include <book/deviceio.h>
#include <share/string.h>
#include <book/task.h>
#include <share/math.h>
#include <share/const.h>
#include <book/vmarea.h>

/*8 bit main status registers*/
#define	DISK_STATUS_BUSY		0x80	//Disk busy
#define	DISK_STATUS_READY	0x40	//Disk ready for 
#define	DISK_STATUS_WREER	0x20	//Disk error
#define	DISK_STATUS_SEEK		0x10	//Seek end
#define	DISK_STATUS_DRQ		0x08	//Request data
#define	DISK_STATUS_ECC		0x04	//ECC check error
#define	DISK_STATUS_INDEX	0x02	//Receive index
#define	DISK_STATUS_ERR		0x01	//Command error

/*AT disk controller command*/
#define	ATA_CMD_RESTORE		0x10	//driver retsore
#define	ATA_CMD_READ		0x20	//Read sector,24位寻址
#define	ATA_CMD_WRITE		0x30	//Write sector,24位寻址
#define	ATA_CMD_READ_24		0x20	//Read sector,24位寻址
#define	ATA_CMD_WRITE_24	0x30	//Write sector,24位寻址
#define ATA_CMD_READ_48		0x24	/* 48位寻址 */
#define ATA_CMD_WRITE_48	0x34	/* 48位寻址 */
#define	ATA_CMD_VERIFY		0x40	//Sector check
#define	ATA_CMD_FORMAT		0x50	//Format track
#define	ATA_CMD_INIT		0x60	//Controller init
#define	ATA_CMD_SEEK		0x70	//Seek operate
#define	ATA_CMD_DIAGNOSE	0x90	//Driver diagnosis command
#define	ATA_CMD_SPECIFY		0x91	//Setting up driver parameters
#define ATA_CMD_IDENTIFY	0xEC	//Get disk information

/*device register*/
#define ATA_DEV_MBS		0xA0	//bit 7 and 5 are 1
#define ATA_DEV_LBA		0x40	//chose Lba way
#define ATA_DEV_DEV		0x10	//device master or slaver

#define MAX_DISK_NR		4	//最大的磁盘数量

/* IO参数 */
#define DISK_TYPE_MASTER	0
#define DISK_TYPE_SLAVE		1

#define	CHANNEL0_PORT_BASE	0x1F0
#define	CHANNEL1_PORT_BASE	0x170

#define PORT_DATA(channel) 			(channel->portBase + 0)
#define PORT_FEATURE(channel) 		(channel->portBase + 1)
#define PORT_ERROR(channel) 		PORT_FEATURE(channel)
#define PORT_SECTOR_CNT(channel) 	(channel->portBase + 2)
#define PORT_SECTOR_LOW(channel) 	(channel->portBase + 3)
#define PORT_SECTOR_MID(channel) 	(channel->portBase + 4)
#define PORT_SECTOR_HIGH(channel) 	(channel->portBase + 5)
#define PORT_DEVICE(channel) 		(channel->portBase + 6)
#define PORT_STATUS(channel) 		(channel->portBase + 7)
#define PORT_CMD(channel) 			PORT_STATUS(channel)

#define PORT_ALT_STATUS(channel) 	(channel->portBase + 0x206)
#define PORT_CTL(channel) 			PORT_ALT_STATUS(channel)

#define DISK_NR_ADDR		0X80000475

/* 请求队列 */
struct RequsetQueue {
	struct WaitQueue waitQueueList;	/* 等待队列链表 */
	
	struct BlockBufferNode *inUsing;	/* 正在处理的需求 */
	int BlockRequestCount;		/* 需求数量 */
};

/*
键盘的私有数据
*/
struct IDE_Private {
	unsigned short portBase;		/* 端口的基地址 */
	unsigned char chanelType;	/* 表明主通道还是从通道 */
	unsigned char diskType;		/* 表明主盘还是从盘 */

	unsigned char isLba48;		/* 是否为lba48的寻址模式 */ 

	/* 磁盘信息 */
	struct Disk_Identify_Info *info;	
	/* 请求队列 */
	struct RequsetQueue requestQueue;
};

/*
块缓冲节点的结构体
*/
struct BlockBufferNode {
	unsigned int count;		/* 扇区的数量 */
	unsigned char cmd;		/* 操作命令 */
	unsigned long lba;		/* 扇区的lba地址 */
	unsigned char *buffer;	/* 缓冲区 */
	
	/* 要处理的中断: device */
	void (*Handler)(struct IDE_Private *);	

	/* 块缓冲区的等待队列 */
	struct WaitQueue waitQueue;	/* 等待队列 */
};

/* 键盘的私有数据 */
PRIVATE struct IDE_Private idePrivateTable[MAX_DISK_NR];

/* 扫描到的磁盘数 */
unsigned char ideDiskFound;

/**
 * SendCmd - 发送命令
 * @device: 要发送命令的设备
 * 
 * 向硬盘发送一个操作命令
 */
PRIVATE void SendCmd(struct IDE_Private *device)
{
	/* 先获取一个节点的等待队列 */
	struct WaitQueue *waitQueue = ListFirstOwner(
		&device->requestQueue.waitQueueList.waitList, struct WaitQueue, waitList);

	/* 通过等待队列获取节点 */
	struct BlockBufferNode *node = ListOwner(waitQueue, struct BlockBufferNode, waitQueue);
	/* 设置当前正在处理的节点 */
	device->requestQueue.inUsing = node;

	/* 减少一个请求 */
	device->requestQueue.BlockRequestCount--;
	
	enum InterruptStatus oldStatus = InterruptDisable();

	/* 执行命令操作 */
	switch (node->cmd)
	{
	case ATA_CMD_READ:

		/* 是否支持lba48寻址 */
		if (device->isLba48) {
			//printk("is lba48.\n");
			/* 支持lba48寻址 */
			/* 设置设备 */
			Out8(PORT_DEVICE(device), ATA_DEV_MBS | ATA_DEV_LBA | 
			(device->diskType == DISK_TYPE_SLAVE ? ATA_DEV_DEV : 0));

			/* 设置写入补偿，写入一次可以设定一次地址 */
			Out8(PORT_FEATURE(device), 0);

			/* 设置扇区数（高8位） */
			Out8(PORT_SECTOR_CNT(device), (node->count >> 8) & 0xff);
			
			/* 设置扇区lba(高24位) */
			Out8(PORT_SECTOR_LOW(device), (node->lba >> 24) & 0xff);

			/* 根据不同的数据宽度设定lba的高位 */
			#ifdef CONFIG_ARCH_32
				Out8(PORT_SECTOR_MID(device), 0);
				Out8(PORT_SECTOR_HIGH(device), 0);
			#elif CONFIG_ARCH_64
				Out8(PORT_SECTOR_MID(device), (node->lba >> 32) & 0xff);
				Out8(PORT_SECTOR_HIGH(device), (node->lba >> 40) & 0xff);
			#endif
			
			/* 设置写入补偿，写入一次可以设定一次地址 */
			Out8(PORT_FEATURE(device), 0);

			/* 设置扇区数（低8位） */
			Out8(PORT_SECTOR_CNT(device), node->count & 0xff);
			
			/* 设置扇区lba(低24位) */
			Out8(PORT_SECTOR_LOW(device), node->lba & 0xff);
			Out8(PORT_SECTOR_MID(device), (node->lba >> 8) & 0xff);
			Out8(PORT_SECTOR_HIGH(device), (node->lba >> 16) & 0xff);
			
			/* 扇区lba和扇区数设置完毕 */
			
			/* 等待磁盘控制器处于准备状态 */
			while (!(In8(PORT_STATUS(device)) & DISK_STATUS_READY))
				CpuNop();

			/* 发送命令 */
			Out8(PORT_CMD(device), ATA_CMD_READ_48);

		} else {
			/* 不支持lba48寻址 */
			//printk("is lba24.\n");
			/* 设置设备 */
			Out8(PORT_DEVICE(device), ATA_DEV_MBS | ATA_DEV_LBA | 
				(device->diskType == DISK_TYPE_SLAVE ? ATA_DEV_DEV : 0));
			
			/* 设置扇区数 */
			Out8(PORT_SECTOR_CNT(device), node->count & 0xff);
			
			/* 设置扇区lba */
			Out8(PORT_SECTOR_LOW(device), node->lba & 0xff);
			Out8(PORT_SECTOR_MID(device), (node->lba >> 8) & 0xff);
			Out8(PORT_SECTOR_HIGH(device), (node->lba >> 16) & 0xff);
			
			/* 设置lba的24~27位（位于device寄存器的0~3位） */
			Out8(PORT_DEVICE(device), ATA_DEV_MBS | ATA_DEV_LBA | 
				(device->diskType == DISK_TYPE_SLAVE ? ATA_DEV_DEV : 0) |
				((node->lba >> 24) & 0x0f));
			
			/* 等待磁盘控制器处于准备状态 */
			while (!(In8(PORT_STATUS(device)) & DISK_STATUS_READY))
				CpuNop();

			/* 发送命令 */
			Out8(PORT_CMD(device), ATA_CMD_READ_24);
		}
		break;
	case ATA_CMD_WRITE:

		/* 是否支持lba48寻址 */
		if (device->isLba48) {
			//printk("is lba48.\n");
			/* 支持lba48寻址 */
			/* 设置设备 */
			Out8(PORT_DEVICE(device), ATA_DEV_MBS | ATA_DEV_LBA | 
					(device->diskType == DISK_TYPE_SLAVE ? ATA_DEV_DEV : 0));

			/* 设置写入补偿，写入一次可以设定一次地址 */
			Out8(PORT_FEATURE(device), 0);

			/* 设置扇区数（高8位） */
			Out8(PORT_SECTOR_CNT(device), (node->count >> 8) & 0xff);
			
			/* 设置扇区lba(高24位) */
			Out8(PORT_SECTOR_LOW(device), (node->lba >> 24) & 0xff);

			/* 根据不同的数据宽度设定lba的高位 */
			#ifdef CONFIG_ARCH_32
				Out8(PORT_SECTOR_MID(device), 0);
				Out8(PORT_SECTOR_HIGH(device), 0);
			#elif CONFIG_ARCH_64
				Out8(PORT_SECTOR_MID(device), (node->lba >> 32) & 0xff);
				Out8(PORT_SECTOR_HIGH(device), (node->lba >> 40) & 0xff);
			#endif
			
			/* 设置写入补偿，写入一次可以设定一次地址 */
			Out8(PORT_FEATURE(device), 0);

			/* 设置扇区数（低8位） */
			Out8(PORT_SECTOR_CNT(device), node->count & 0xff);
			
			/* 设置扇区lba(低24位) */
			Out8(PORT_SECTOR_LOW(device), node->lba & 0xff);
			Out8(PORT_SECTOR_MID(device), (node->lba >> 8) & 0xff);
			Out8(PORT_SECTOR_HIGH(device), (node->lba >> 16) & 0xff);
			
			/* 扇区lba和扇区数设置完毕 */
			
			/* 等待磁盘控制器处于准备状态 */
			while (!(In8(PORT_STATUS(device)) & DISK_STATUS_READY))
				CpuNop();

			/* 发送命令 */
			Out8(PORT_CMD(device), ATA_CMD_WRITE_48);

			/* 等待硬盘请求写入数据 */
			while (!(In8(PORT_STATUS(device)) & DISK_STATUS_DRQ))
				CpuNop();

			/* 写入数据到数据端口，写完之后，等待硬盘写入数据，完成之后会产生中断 */
			PortWrite(PORT_DATA(device), node->buffer, SECTOR_SIZE);

		} else {
			/* 不支持lba48寻址 */
			
			//printk("is lba24.\n");
			/* 设置设备 */
			Out8(PORT_DEVICE(device), ATA_DEV_MBS | ATA_DEV_LBA | 
				(device->diskType == DISK_TYPE_SLAVE ? ATA_DEV_DEV : 0));
			
			/* 设置扇区数 */
			Out8(PORT_SECTOR_CNT(device), node->count & 0xff);
			
			/* 设置扇区lba */
			Out8(PORT_SECTOR_LOW(device), node->lba & 0xff);
			Out8(PORT_SECTOR_MID(device), (node->lba >> 8) & 0xff);
			Out8(PORT_SECTOR_HIGH(device), (node->lba >> 16) & 0xff);
			
			/* 设置lba的24~27位（位于device寄存器的0~3位） */
			Out8(PORT_DEVICE(device), ATA_DEV_MBS | ATA_DEV_LBA | 
				(device->diskType == DISK_TYPE_SLAVE ? ATA_DEV_DEV : 0) |
				((node->lba >> 24) & 0x0f));
			
			/* 等待磁盘控制器处于准备状态 */
			while (!(In8(PORT_STATUS(device)) & DISK_STATUS_READY))
				CpuNop();

			/* 发送命令 */
			Out8(PORT_CMD(device), ATA_CMD_WRITE_24);

			/* 等待硬盘请求写入数据 */
			while (!(In8(PORT_STATUS(device)) & DISK_STATUS_DRQ))
				CpuNop();

			/* 写入数据到数据端口，写完之后，等待硬盘写入数据，完成之后会产生中断 */
			PortWrite(PORT_DATA(device), node->buffer, SECTOR_SIZE);
		}
		break;


 	case ATA_CMD_IDENTIFY:
		/* 设置设备 */
		Out8(PORT_DEVICE(device), ATA_DEV_MBS | ATA_DEV_LBA | 
				(device->diskType == DISK_TYPE_SLAVE ? ATA_DEV_DEV : 0));
		
		/* 设置写入补偿 */
		Out8(PORT_FEATURE(device), 0);
			
		/* 设置扇区数 */
		Out8(PORT_SECTOR_CNT(device), node->count & 0xff);
		
		/* 设置扇区lba */
		Out8(PORT_SECTOR_LOW(device), node->lba & 0xff);
		Out8(PORT_SECTOR_MID(device), (node->lba >> 8) & 0xff);
		Out8(PORT_SECTOR_HIGH(device), (node->lba >> 16) & 0xff);

		/* 等待硬盘准备好 */
		while (!(In8(PORT_STATUS(device)) & DISK_STATUS_READY))
			CpuNop();		

		/* 发送命令 */
		Out8(PORT_CMD(device), node->cmd);
		break;


	default:
		break;
 	}
	InterruptSetStatus(oldStatus);
}

/**
 * EndRequest - 结束请求
 * @device: 要结束的设备
 * 
 * 结束一个请求，唤醒在请求中等待的任务，并释放节点
 */
PRIVATE void EndRequest(struct IDE_Private *device)
{
	if (device == NULL)
		printk(PART_ERROR "EndRequest: device error!\n");

	struct BlockBufferNode *node = device->requestQueue.inUsing;

	/* 通知等待者以及处理完毕 */
	if (node->waitQueue.task->status == TASK_BLOCKED) {
		TaskUnblock(node->waitQueue.task);
	}

	/* 释放当前节点 */
	kfree(device->requestQueue.inUsing);

	/* 把使用中的节点置空 */
	device->requestQueue.inUsing = NULL;

	/* 如果还有就继续发送命令 */
	if (device->requestQueue.BlockRequestCount)
		SendCmd(device);
}

/**
 * IDE_AssistHandler - 任务协助处理函数
 * @data: 传递的数据
 */
PRIVATE void IDE_AssistHandler(unsigned int data)
{
	struct IDE_Private *device = (struct IDE_Private *)data;

	struct BlockBufferNode *node = device->requestQueue.inUsing;
	
	node->Handler(device);
	//Panic("test");
	
}

/* 硬盘的任务协助 */
PRIVATE struct TaskAssist ideAssistTable[MAX_DISK_NR];

/**
 * IDE_ReadHandler - 读中断处理
 * @device: 要处理的设备
 */
PRIVATE void IDE_ReadHandler(struct IDE_Private *device)
{
	struct BlockBufferNode *node = device->requestQueue.inUsing;
	//printk("read occur!\n");
	/* 先检测运行状态是否出错 */
	if (In8(PORT_STATUS(device)) & DISK_STATUS_ERR)
		printk(PART_ERROR "IDE_ReadHandler: error %x\n",In8(PORT_ERROR(device)));
	else {
		/* 读取数据到缓冲区 */
		PortRead(PORT_DATA(device),node->buffer, SECTOR_SIZE);
	}
	/* 结束这个请求 */
	EndRequest(device);
}


/**
 * IDE_WriteHandler - 写中断处理
 * @device: 要处理的设备
 */
PRIVATE void IDE_WriteHandler(struct IDE_Private *device)
{
	
	/* 先检测运行状态是否出错 */
	if (In8(PORT_STATUS(device)) & DISK_STATUS_ERR)
		printk(PART_ERROR "IDE_WriteHandler: error %x\n",In8(PORT_ERROR(device)));

	/* 结束这个请求 */
	EndRequest(device);
}


/**
 * IDE_OtherHandler - 其它中断处理
 * @device: 要处理的设备
 */
PRIVATE void IDE_OtherHandler(struct IDE_Private *device)
{
	
	struct BlockBufferNode *node = device->requestQueue.inUsing;
	
	/* 先检测运行状态是否出错 */
	if (In8(PORT_STATUS(device)) & DISK_STATUS_ERR)
		printk(PART_ERROR "IDE_OtherHandler: error %x\n",In8(PORT_ERROR(device)));
	else
		PortRead(PORT_DATA(device),node->buffer, SECTOR_SIZE);

	/* 结束这个请求 */
	EndRequest(device);
}

/**
 * KeyboardHnadler - 时钟中断处理函数
 * @irq: 中断号
 * @data: 中断的数据
 */
PRIVATE void IDE_Handler(unsigned int irq, unsigned int data)
{
	//printk("int occur %d\n", irq);
	int minorID = 0;
	/* 先通过irq获取通道 */
	minorID = (irq - IRQ14) * 2;

	/* 再通过DEVICE寄存器获取主从设备 */
	unsigned char deviceReg;

	/* 获取通道 */
	struct IDE_Private *device = &idePrivateTable[minorID];

	if (minorID == 0) {
		/* 主通道 */
		deviceReg =  In8(PORT_DEVICE(device));
	} else if (minorID == 2) {
		/* 从通道 */
		deviceReg =  In8(PORT_DEVICE(device));
	}
	
	/* 根据设备信息查看主从设备 */
	if (deviceReg & ATA_DEV_DEV)
		minorID++;

	/* 如果设备号相等就可以执行，不然就返回 */
	if (minorID != data) 
		return;

	/* ----处理设备操作---- */
	
	/* 获取磁盘设备 */
	device = &idePrivateTable[minorID];
		
	/* 传入设备参数 */
	ideAssistTable[minorID].data = (unsigned int)device;
	
	/* 调度协助 */
	TaskAssistSchedule(&ideAssistTable[minorID]);
}

/**
 * MakeRequest - 生成一个请求
 * @cmd: 命令
 * @pos: 要读写的位置
 * @buffer: 数据存放的缓冲区
 * @count: 扇区数
 * 
 * 把数据生成一个请求，然后通过请求访问磁盘
 */
PRIVATE struct BlockBufferNode *MakeRequest(unsigned char cmd,
		unsigned long pos, unsigned char *buffer, unsigned int count)
{
	/* 分配一个节点 */
	struct BlockBufferNode *node = kmalloc(sizeof(struct BlockBufferNode), GFP_KERNEL);
	if (node == NULL)
		return NULL;

	/* 把当前任务添加到节点的等待队列中 */
	WaitQueueInit(&node->waitQueue, CurrentTask());
	/* 根据命令选择不同处理函数 */
	switch (cmd)
	{
	case ATA_CMD_READ:
		node->Handler = IDE_ReadHandler;
		node->cmd = cmd;
		break;
	
	case ATA_CMD_WRITE:
		node->Handler = IDE_WriteHandler;
		node->cmd = cmd;
		break;
	
	default:
		node->Handler = IDE_OtherHandler;
		node->cmd = cmd;
		break;
	}

	/* 设置节点操作信息 */
	node->lba = pos;
	node->count = count;
	node->buffer = buffer;
	
	return node;
}

/**
 * AddRequest - 添加一个请求
 * @device: 要请求的设备
 * @node: 请求节点
 */
PRIVATE void AddRequest(struct IDE_Private *device, struct BlockBufferNode *node)
{
	/* 在这里面做I/O调度排序 */
	
	/* 现在就简单的把新的请求添加到最后面，用FIFO方式
	把节点的等待队列添加到设备的等待队列链表上 */
	
	ListAddTail(&node->waitQueue.waitList, &device->requestQueue.waitQueueList.waitList);
	
	/* 现在多了一个请求 */
	device->requestQueue.BlockRequestCount++;
}

/**
 * SubmitRequest - 提交请求
 * @device: 要请求的设备
 * @node: 请求节点
 * 
 * 把一个请求提交给硬盘驱动器，让它处理请求
 */
PRIVATE void SubmitRequest(struct IDE_Private *device, struct BlockBufferNode *node)
{
	
	/* 把需求添加到队列中 */
	AddRequest(device, node);
	
	/* 如果没有正在处理的节点，就立即处理当前节点 */
	if (device->requestQueue.inUsing == NULL)
		SendCmd(device);	/* 发送命令给磁盘 */
	
}

/**
 * WaitRequestEnd - 等待请求结束
 * @device: 要等待的设备
 * 
 * 等待中，当前任务可能会被阻塞
 */
PRIVATE void WaitRequestEnd(struct IDE_Private *device)
{

	/* 做一个小循环，拖延时间 */
	int i;
	for (i = 0; i < 10000; i++)
		CpuNop();
	
	/* 如果当前的请求还没有结束，就阻塞等待，等待结束后唤醒自己
	如果当前的请求在等待之前就已经结束了，那么就直接返回
	 */
	if (device->requestQueue.inUsing) {	
		/* 如果过了一会儿还没有被处理，就让出处理权限，等待被唤醒 */	
		TaskBlock(TASK_BLOCKED);
	}
}

//#define CONFIG_DEBUG_IDENTIFY
/**
 * IDE_GetIdentify - 获取磁盘信息
 * @deviceID
 * 
 * 获取磁盘信息，并根据信息设定磁盘。例如是否支持lba48寻址
 */
PRIVATE int IDE_GetIdentify(unsigned int deviceID)
{
	//int i;
	/* 分配一个扇区来保存信息 */
	int minor = DeviceGetMinor(deviceID);
	if (minor == -1) {
		printk(PART_ERROR "device get minor failed!\n");
		return -1;
	}

	struct IDE_Private *device = &idePrivateTable[minor];

	device->info = kmalloc(SECTOR_SIZE, GFP_KERNEL);
	memset(device->info, 0, SECTOR_SIZE);

	/* 通过identify命令获取磁盘信息 */
	if (DeviceIoctl(deviceID, ATA_IO_IDENTIFY, (int)device->info, 0)) {
		printk(PART_ERROR "device %d IOCTL failed!\n", deviceID); 
		kfree(device->info);
		return -1;
	}
	#ifdef CONFIG_DEBUG_IDENTIFY

	printk("\nSerial Number:");
	
	for (i = 0; i < 10; i++) {
		printk("%c%c", (device->info->Serial_Number[i] >> 8) & 0xff,
			device->info->Serial_Number[i] & 0xff);
	}

	printk("\nFireware Version:");
	for (i = 0; i < 4; i++) {
		printk("%c%c", (device->info->Firmware_Version[i] >> 8) & 0xff,
			device->info->Firmware_Version[i] & 0xff);
	}

	printk("\nModel Number:");
	for (i = 0; i < 20; i++) {
		printk("%c%c", (device->info->Model_Number[i] >> 8) & 0xff,
			device->info->Model_Number[i] & 0xff);
	}
	
	printk("\nTotal Sector(LBA 28):");

	printk("%d", ((int)device->info->Addressable_Logical_Sectors_for_28[1] << 16) + 
			(int)device->info->Addressable_Logical_Sectors_for_28[0]);
	
	printk("\nLBA supported:%s ",(device->info->Capabilities0 & 0x0200) ? "Yes" : "No");
	printk("\nLBA48 supported:%s\n",((device->info->Cmd_feature_sets_supported1 & 0x0400) ? "Yes" : "No"));

	#endif

	/* 检查是否支持LBA48,如果支持才可以用48的读写。不然就用24的读写 */
	if (device->info->Cmd_feature_sets_supported1 & 0x0400) {

		/* 是支持LBA48的模式的 */
		if (DeviceIoctl(deviceID, ATA_IO_LBA_MODE, DISK_LBA_48, 0)) {
			printk(PART_ERROR "device %d set lba48 failed!\n", deviceID);
			return -1;
		}
	}
	return 0;
}

/**
 * IDE_Open - 打开硬盘
 * @deviceEntry: 设备项
 * @name: 设备名
 * @mode: 打开模式
 */
PRIVATE int IDE_Open(struct DeviceEntry *deviceEntry, char *name, char *mode)
{
	
	return 0;
}

/**
 * IDE_Close - 关闭硬盘
 * @deviceEntry: 设备项
 */
PRIVATE int IDE_Close(struct DeviceEntry *deviceEntry)
{
	
	return 0;
}

/**
 * IDE_Ioctl - 硬盘的IO控制
 * @deviceEntry: 设备项
 * @cmd: 命令
 * @arg1: 参数1
 * @arg2: 参数2
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int IDE_Ioctl(struct DeviceEntry *deviceEntry, int cmd, int arg1, int arg2)
{
	struct BlockBufferNode *node = NULL;

	struct IDE_Private *device = (struct IDE_Private *)deviceEntry->device;

	/* 根据IO命令进行不同的操作 */
	switch (cmd)
	{
	case ATA_IO_IDENTIFY:
		/* 生成一个请求节点 */
		node = MakeRequest(ATA_CMD_IDENTIFY, 0, (unsigned char *)arg1, 0);
		if (node == NULL)
			return -1; 

		/* 提交请求 */
		SubmitRequest(device, node);
		
		/* 等待中断响应结束 */
		WaitRequestEnd(device);
		
		break;
	case ATA_IO_LBA_MODE:
		/* 如果参数正确才进行设置 */
		if (arg1 == DISK_LBA_24 || arg1 == DISK_LBA_48) {
			device->isLba48 = arg1;
		}

		break;

	case ATA_IO_SECTORS:
		*((unsigned int *)arg1) = (((int)device->info->Addressable_Logical_Sectors_for_28[1] << 16) + 
			(int)device->info->Addressable_Logical_Sectors_for_28[0]);
		break;
			
	
	default:
		break;
	}

	return 0;
}

/**
 * IDE_Read - 读取硬盘数据
 * @deviceEntry: 设备项
 * @buf: 读取到缓冲区
 * @count: 操作的扇区数
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int IDE_Read(struct DeviceEntry *deviceEntry, unsigned int lba, void *buffer, unsigned int count)
{
	/* 进行读取操作 */
	struct BlockBufferNode *node = NULL;

	struct IDE_Private *device = (struct IDE_Private *)deviceEntry->device;

	unsigned char *buf = (unsigned char *)buffer;

	/* 由于读取的时候向磁盘写入多个扇区读取会出现bug，所以在这里，我们就
	用内部循环读取的方式，也就是说我们一次性请求就只读取一个扇区，然后多请求
	几次，就可以实现一次性读取多个连续的扇区了。*/
	int i = 0;
	
	do {	
		/* 生成一个请求节点 */
		node = MakeRequest(ATA_CMD_READ, lba, buf, 1);
		if (node == NULL)
			return -1; 
		//printk("make request\n");
			
		/* 提交请求 */
		SubmitRequest(device, node);
		//printk("SubmitRequest\n");
			
		/* 等待中断响应结束 */
		WaitRequestEnd(device);
		//printk("WaitRequestEnd\n");
			
		/* 指向下一个扇区buffer */
		buf += SECTOR_SIZE;
		/* 指向下一个扇区 */
		lba++;
		i++;
	} while (i < count);
	
	return 0;
}

/**
 * IDE_Write - 写入数据到硬盘
 * @deviceEntry: 设备项
 * @buf: 要写入的缓冲区
 * @count: 操作的扇区数
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int IDE_Write(struct DeviceEntry *deviceEntry, unsigned int lba, void *buffer, unsigned int count)
{
	/* 进行写入操作 */
	struct BlockBufferNode *node = NULL;

	struct IDE_Private *device = (struct IDE_Private *)deviceEntry->device;

	int i = 0;
	
	do {
		/* 生成一个请求节点 */
		node = MakeRequest(ATA_CMD_WRITE, lba, (unsigned char *)buffer, 1);
		if (node == NULL)
			return -1; 
			
		/* 提交请求 */
		SubmitRequest(device, node);
		
		/* 等待中断响应结束 */
		WaitRequestEnd(device);
		
		/* 指向下一个扇区buffer */
		buffer += SECTOR_SIZE;
		/* 指向下一个扇区 */
		lba++;
		i++;
	} while (i < count);

	return 0;
}

/**
 * IDE_Init - 初始化ide硬盘驱动
 * @deviceEntry: 设备项
 */
PRIVATE int IDE_Init(struct DeviceEntry *deviceEntry)
{
	
	struct IDE_Private *priv = (struct IDE_Private *)deviceEntry->device;

	/* 如果次设备号错误，就直接返回 */
	if (deviceEntry->minor < 0 || deviceEntry->minor >= MAX_DISK_NR)
		return -1;
	
	/* 根据设备号选择主从通道以及端口基地址 */
	if (deviceEntry->minor <= 1) {
		priv->chanelType = 0;	/* 是主通道 */
		priv->portBase = CHANNEL0_PORT_BASE;	/* 是主通道端口基地址 */
		
	} else {
		priv->chanelType = 1;	/* 是从通道 */
		priv->portBase = CHANNEL1_PORT_BASE;	/* 是从通道端口基地址 */	
	}

	/* 根据设备号选择主从盘 */
	if (deviceEntry->minor % 2 == 0) {
		priv->diskType = DISK_TYPE_MASTER;	/* 是主盘 */
	} else {
		priv->diskType = DISK_TYPE_SLAVE;	/* 是从盘 */
	}

	/* 初始化寻址模式 */
	priv->isLba48 = DISK_LBA_24;		/* 默认不是lba48寻址 */

	/* 初始化需求队列 */
	WaitQueueInit(&priv->requestQueue.waitQueueList, NULL);
	priv->requestQueue.inUsing = NULL;
	priv->requestQueue.BlockRequestCount = 0;
	
	/* 初始化任务协助 */
	TaskAssistInit(&ideAssistTable[deviceEntry->minor], IDE_AssistHandler, 0);

	/* 注册时钟中断并打开中断 */	
	if (deviceEntry->irq == IRQ14) {
		RegisterIRQ(deviceEntry->irq, deviceEntry->Handler, IRQF_DISABLED | IRQF_SHARED, "IRQ14", deviceEntry->name, deviceEntry->minor);
	} else if (deviceEntry->irq == IRQ15) {
		RegisterIRQ(deviceEntry->irq, deviceEntry->Handler, IRQF_DISABLED | IRQF_SHARED, "IRQ15", deviceEntry->name, deviceEntry->minor);
	}
	return 0;
}

/**
 * 设备的操作控制
 */
PRIVATE struct DeviceOperate operate = {
	.Init =  &IDE_Init, 
	.Open = &IDE_Open, 
	.Close = &IDE_Close, 
	.Read = &IDE_Read, 
	.Write = &IDE_Write,
	.Getc = (void *)&IoNull, 
	.Putc = (void *)&IoNull, 
	.Ioctl = &IDE_Ioctl, 
};

/**
 * InitIDE_Driver - 初始化IDE硬盘驱动
 */
PUBLIC void InitIDE_Driver()
{
	PART_START("IDE Driver");

	/* 获取已经配置了的硬盘数量 */
	ideDiskFound = *((unsigned char *)DISK_NR_ADDR);
	printk(PART_TIP "ide disks %d\n", ideDiskFound);
	ASSERT(ideDiskFound > 0);

	/* 根据磁盘数注册磁盘 */
	switch (ideDiskFound)
	{
	case 4:
		/* 注册 */
		RegisterDevice(DEVICE_HDD0, 3, "ide3", 
			&operate, &idePrivateTable[3], &IDE_Handler, IRQ15);
		/* 初始化 */
		DeviceInit(DEVICE_HDD3);
		
		/* 打开 */
		DeviceOpen(DEVICE_HDD3, "ide3", "null");

		//DeviceLookOver(DEVICE_HDD3);
	
	case 3:
		RegisterDevice(DEVICE_HDD0, 2, "ide2", 
			&operate, &idePrivateTable[2], &IDE_Handler, IRQ15);
		
		/* 初始化 */
		DeviceInit(DEVICE_HDD2);
		
		/* 打开 */
		DeviceOpen(DEVICE_HDD2, "ide2", "null");
	
		//DeviceLookOver(DEVICE_HDD2);
    
	case 2:
		RegisterDevice(DEVICE_HDD0, 1, "ide1", 
			&operate, &idePrivateTable[1], &IDE_Handler, IRQ14);

		/* 初始化 */
		DeviceInit(DEVICE_HDD1);
		
		/* 打开 */
		DeviceOpen(DEVICE_HDD1, "ide1", "null");
	
		//DeviceLookOver(DEVICE_HDD1);
    
	case 1:
		RegisterDevice(DEVICE_HDD0, 0, "ide0", 
			&operate, &idePrivateTable[0], &IDE_Handler, IRQ14);

		/* 初始化 */
		DeviceInit(DEVICE_HDD0);
		
		/* 打开 */
		DeviceOpen(DEVICE_HDD0, "ide0", "null");

		//DeviceLookOver(DEVICE_HDD0);
    
		break;
	default:
		break;
	}

	/* 等初始化全部完毕后再获取信息 */
	switch (ideDiskFound)
	{
	case 4:
		IDE_GetIdentify(DEVICE_HDD3);
	case 3:
		IDE_GetIdentify(DEVICE_HDD2);
	case 2:
		IDE_GetIdentify(DEVICE_HDD1);
	case 1:
		IDE_GetIdentify(DEVICE_HDD0);
		break;
	default:
		break;
	}
	/*
	unsigned int deviceID = DEVICE_HDD0;
	unsigned int lba = 0;
	unsigned int sectors = 1;
	
	char *buf = kmalloc(SECTOR_SIZE*sectors, GFP_KERNEL);
	memset(buf, 0, SECTOR_SIZE*sectors);

	if (DeviceRead(deviceID, lba, (char *)buf, sectors)) {
		printk("oh! no! we read from disk failed!\n");
	}
	
	printk("oh! It's the data: %x %x\n", buf[0], buf[511]);
	
	memset(buf, 0x5a, SECTOR_SIZE*sectors);

	if (DeviceWrite(deviceID, lba, (char *)buf, sectors)) {
		printk("oh! no! we write to disk failed!\n");
	}
	memset(buf, 0, SECTOR_SIZE*sectors);
	
	if (DeviceRead(deviceID, lba, (char *)buf, sectors)) {
		printk("oh! no! we read from disk failed!\n");
	}
	printk("oh! It's the data: %x %x\n", buf[0], buf[511]);
			
	kfree(buf);
	
	Panic("TEST");
*/
    PART_END();
}
