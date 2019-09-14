/*
 * file:		driver/char/mouse.c
 * auther:		Jason Hu
 * time:		2019/8/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/interrupt.h>
#include <driver/mouse.h>
#include <book/ioqueue.h>
#include <hal/char/keyboard.h>
#include <share/string.h>

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4
#define KEYCMD_ENABLE_MOUSE_INTFACE			0xa8

/* 鼠标数据包 */
struct MousePacket {
	/* 第一字节数据内容
	7: y overflow, 6: x overflow, 5: y sign bit, 4: x sign bit,
	3: alawys 1  , 2: middle btn, 1: right btn , 0: left btn  .
	*/
	unsigned char byte0;
	unsigned char byte1;		// x 移动值
	unsigned char byte2;		// y 移动值
	unsigned char byte3;		// z 移动值
	/*
	7: y always 0, 6: always 0, 5: 鼠标第5键, 4: 鼠标第4键,
	3: 滚轮水平左滚动, 2: 滚轮水平右滚动, 1: 滚轮垂直下滚动, 0: 滚轮垂直上滚动.
	*/
	unsigned char byte4;
};

/*
键盘的私有数据
*/
struct MousePrivate {
	struct MousePacket mousePacket;		// 鼠标的数据包
	unsigned char rowData;	// 获取的原书数据
	char phase;		// 解析步骤
	int xIncrease;	// x增长值
	int yIncrease;	// y增长值
	int button;		// 按键值
	int x;
	int y;
};

/* 键盘的私有数据 */
PRIVATE struct MousePrivate mousePrivate;

/**
 * GetByteFromBuf - 从键盘缓冲区中读取下一个字节
 */
PRIVATE unsigned char GetRowData()       
{
	unsigned char rowData = mousePrivate.rowData;
	mousePrivate.rowData = 0;
	return rowData;
}

/**
 * AnalysisMouse - 解析鼠标数据
 * 
 * 返回-1，表明解析出错。
 * 返回0，表明解析成功，但还没有获取完整的数据包
 * 返回1，表明解析成功，获取了完整的数据包
*/
PRIVATE int AnalysisMouse()
{
	unsigned char rowData = GetRowData();
	if (mousePrivate.phase == 0) {
		if (rowData == 0xfa) {
			mousePrivate.phase++;
		}
		return 0;
	}
	if (mousePrivate.phase == 1) {
		if ((rowData & 0xc8) == 0x08) {
			mousePrivate.mousePacket.byte0 = rowData;
			mousePrivate.phase++;
			
		}
		return 0;
	}
	if (mousePrivate.phase == 2) {
		mousePrivate.mousePacket.byte1 = rowData;
		mousePrivate.phase++;
		return 0;
	}
	if (mousePrivate.phase == 3) {
		mousePrivate.mousePacket.byte2 = rowData;
		mousePrivate.phase = 1;

		//printk("(B:%x, X:%d, Y:%d)\n", mousePrivate.mousePacket.byte0, mousePrivate.mousePacket.byte1, mousePrivate.mousePacket.byte2);
		
		/* 只获取低3位，也就是按键按下的位 */
		mousePrivate.button = mousePrivate.mousePacket.byte0 & 0x07;
		mousePrivate.xIncrease = mousePrivate.mousePacket.byte1;
		mousePrivate.yIncrease = mousePrivate.mousePacket.byte2;

		/* 如果x有符号，那么就添加上符号 */
		if ((mousePrivate.mousePacket.byte0 & 0x10) != 0) {
			mousePrivate.xIncrease |= 0xffffff00;
		}
		
		/* 如果y有符号，那么就添加上符号 */
		if ((mousePrivate.mousePacket.byte0 & 0x20) != 0) {
			mousePrivate.yIncrease |= 0xffffff00;
		}

		/* y增长是反向的，所以要取反 */
		mousePrivate.yIncrease = -mousePrivate.yIncrease;
		//printk("[x:%d, y:%d](%x)\n", mousePrivate.xIncrease, mousePrivate.yIncrease, mousePrivate.button);
		
		return 1;
	}
	return -1; 
}

/**
 * TaskAssistHandler - 任务协助函数
 * @data: 传递的数据
 */
PRIVATE void TaskAssistHandler(unsigned int data)
{
	/* 获取了一次完整的数据包 */
	if(AnalysisMouse()){
		mousePrivate.x += mousePrivate.xIncrease;
		mousePrivate.y += mousePrivate.yIncrease;
		
		if (mousePrivate.x < 0) {
			mousePrivate.x = 0;
		}
		if (mousePrivate.x > 800) {
			mousePrivate.x = 800;
		}
		
		if (mousePrivate.y < 0) {
			mousePrivate.y = 0;
		}
		
		if (mousePrivate.y >600) {
			mousePrivate.y = 600;
		}
		printk("(%d, %d)\n", mousePrivate.x, mousePrivate.y);
	}
}

/* 键盘任务协助 */
PRIVATE struct TaskAssist mouseAssist;

/**
 * KeyboardHnadler - 时钟中断处理函数
 * @irq: 中断号
 * @data: 中断的数据
 */
PRIVATE void MouseHnadler(unsigned int irq, unsigned int data)
{
	/* 先从硬件获取按键数据 */
	//HalRead("keyboard", (unsigned char *)&keyboardPrivate.rowData, 4);
	//mousePrivate.rowData = KEYBOARD_HAL_GET_DATA();
	mousePrivate.rowData = KEYBOARD_HAL_GET_DATA();

	//printk("%x->", mousePrivate.rowData);
	/* 调度任务协助 */
	TaskAssistSchedule(&mouseAssist);

}

/**
 * InitMouseDriver - 初始化鼠标驱动
 */
PUBLIC void InitMouseDriver()
{
	PART_START("Mouse driver");

	/* 初始化私有数据 */
	memset(&mousePrivate.mousePacket, 0, sizeof(struct MousePacket));

	mousePrivate.rowData = 0;
	mousePrivate.xIncrease = 0;
	mousePrivate.yIncrease = 0;
	mousePrivate.button = 0;
	mousePrivate.x = 0;
	mousePrivate.y = 0;
	
	//打开时钟硬件抽象
	//HalOpen("mouse");

	/* 初始化任务协助 */
	TaskAssistInit(&mouseAssist, TaskAssistHandler, 0);
	
	/* 注册时钟中断并打开中断 */	
	RegisterIRQ(IRQ12_MOUSE, &MouseHnadler, 0, "Mouseirq", "Mouse", 0);

	/* 设置发送数据给鼠标 */
	KeyboardControllerWait();
	Out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	//KeyboardControllerAck();
	
	/* 传递打开鼠标的数据 */
	KeyboardControllerWait();
	Out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	//KeyboardControllerAck();
	mousePrivate.phase = 0;
	
	
	PART_END();
}
