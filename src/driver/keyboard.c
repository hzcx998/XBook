/*
 * file:		driver/char/keyboard.c
 * auther:		Jason Hu
 * time:		2019/8/19
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/interrupt.h>
#include <driver/keyboard.h>
#include <driver/keymap.h>
#include <book/ioqueue.h>
#include <book/device.h>
#include <share/string.h>
#include <book/chr-dev.h>

/* I/O port for keyboard data
Read : Read Output Buffer
Write: Write Input Buffer(8042 Data&8048 Command) 
*/
#define PORT_KEYDAT		0x0060

/* I/O port for keyboard command
Read : Read Status Register
Write: Write Input Buffer(8042 Command) 
*/
#define PORT_KEYCMD		0x0064

#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

#define LED_CODE		0xED		//led操作
#define KBC_ACK			0xFA		//回复码

#define KEYSTA_SEND_NOTREADY	0x02

#define DEVNAME "keyboard"

/* 按键的一些标志 */
#define FLAG_BREAK	0x0080		/* Break Code			*/
#define FLAG_SHIFT_L	0x0200		/* Shift key			*/
#define FLAG_SHIFT_R	0x0400		/* Shift key			*/
#define FLAG_CTRL_L	0x0800		/* Control key			*/
#define FLAG_CTRL_R	0x1000		/* Control key			*/
#define FLAG_ALT_L	0x2000		/* Alternate key		*/
#define FLAG_ALT_R	0x4000		/* Alternate key		*/
#define FLAG_PAD	0x8000		/* keys in num pad		*/

/* raw key value = code passed to tty & MASK_RAW
the value can be found either in the keymap column 0
or in the list below */
#define MASK_RAW	0x01FF		

/*
键盘的私有数据
*/
struct KeyboardPrivate {
	int	codeWithE0;	/* 携带E0的值 */
	int	shiftLeft;	/* l shift state */
	int	shiftRight;	/* r shift state */
	int	altLeft;	/* l alt state	 */
	int	altRight;	/* r left state	 */
	int	ctrlLeft;	/* l ctrl state	 */
	int	ctrlRight;	/* l ctrl state	 */
	int	capsLock;	/* Caps Lock	 */
	int	numLock;	/* Num Lock	 */
	int	scrollLock;	/* Scroll Lock	 */
	int	column;		/* 数据位于哪一列 */
	
	int rowData;	/* 原始数据 */
	int keyData;	/* 解析后的数据 */

	char workMode;	/* 工作模式 */
	struct WaitQueue callerWaitQueue;		/* 调用者等待队列 */
	struct CharDevice *chrdev;	/* 字符设备 */
};

/* 键盘的私有数据 */
PRIVATE struct KeyboardPrivate keyboardPrivate;

/**
 * SetKeyData - 设置字符的数据
 * @key: 键值
 */
PRIVATE void SetKeyData(unsigned int key)
{
	keyboardPrivate.keyData = key;
	/* 唤醒等待队列中的一个任务 */
	WaitQueueWakeUp(&keyboardPrivate.callerWaitQueue);
}

/**
 * GetKeyData - 获取字符的数据
 * 
 * 把键盘值返回给调用者
 */
PRIVATE unsigned int GetKeyData()
{
	unsigned int data = keyboardPrivate.keyData;
	keyboardPrivate.keyData = 0;
	return data;
}

/**
 * KeyboardControllerWait - 等待 8042 的输入缓冲区空
 */ 
PUBLIC void KeyboardControllerWait()
{
	unsigned char stat;

	do {
		stat = In8(PORT_KEYCMD);
	} while (stat & KEYSTA_SEND_NOTREADY);
}

/**
 * KeyboardControllerAck - 等待键盘控制器回复
 */
PUBLIC void KeyboardControllerAck()
{
	unsigned char read;

	do {
		read = In8(PORT_KEYDAT);
	} while ((read =! KBC_ACK));
	
}

/**
 * SetLeds - 设置键盘led灯状态
 */
PRIVATE void SetLeds()
{
	/* 先合成成为一个数据，后面写入寄存器 */
	unsigned char leds = (keyboardPrivate.capsLock << 2) | 
			(keyboardPrivate.numLock << 1) | keyboardPrivate.scrollLock;
	
	/* 数据指向LED_CODE */
	KeyboardControllerWait();
	Out8(PORT_KEYDAT, LED_CODE);
	KeyboardControllerAck();

	/* 写入新的led值 */
	KeyboardControllerWait();
	Out8(PORT_KEYDAT, leds);
	KeyboardControllerAck();
}

/**
 * GetByteFromBuf - 从键盘缓冲区中读取下一个字节
 */
PRIVATE unsigned char GetByteFromBuf()       
{
	unsigned int rowData = keyboardPrivate.rowData;
	keyboardPrivate.rowData = 0;
	return (unsigned char)rowData;
}
/**
 * KeyCharProcess - 对单独的按键进行处理
 * @key: 按键 
 */
PRIVATE void KeyCharProcess(unsigned int key)
{
	/* 没有扩展数据，就直接写入对应的key */
	if (!(key & FLAG_EXT)) {
		
		SetKeyData(key);
	} else {
		/* 有扩展的数据，需要解析 */
		int raw_code = key & MASK_RAW;
		
		switch(raw_code) {
			case ENTER:
				SetKeyData('\n');
				break;
			case BACKSPACE:
				SetKeyData( '\b');
				break;
			case TAB:
				
				break;
			case F1:
				SetKeyData( F1);
				/*if(key & FLAG_altLeft || key & FLAG_altRight){
					select_console(0);
				}*/
				break;
			case F2:
				SetKeyData( F2);
				
				break;
				
			case F3:
				//shft +f4 关闭窗口程序
				SetKeyData( F3);
				
				break;
			case F4:
				//alt +f4 关闭程序
				SetKeyData( F4);
				break;	
			case F5:
				SetKeyData( F5);

				break;	  
			case F6:
				SetKeyData( F6);
				break;	   
			case F7:
				SetKeyData( F7);
				break;	  
			case F8:
				SetKeyData( F8);
				break;	  
			case F9:
				SetKeyData( F9);
				break;	  
			case F10:
				SetKeyData( F10);
				break;	  
			case F11:
				SetKeyData( F11);
				break;	  
			case F12:  
				SetKeyData( F12);
				break;
			case ESC:
				SetKeyData(ESC);
				
				break;	
			case UP:
				SetKeyData(UP);
				
				break;
			case DOWN:
				SetKeyData(DOWN);
				
				break;
			case LEFT:
				
				SetKeyData(LEFT);
				break;
			case RIGHT:
				SetKeyData(RIGHT);
				break;
			case PAGEUP:
				SetKeyData(PAGEUP);
				break;
			case PAGEDOWN:
				SetKeyData(PAGEDOWN);
				break;	
			case HOME:
				SetKeyData(HOME);
				break;	
			case END:
				SetKeyData(END);
				break;	
			case INSERT:
				SetKeyData(INSERT);
				break;	
			case DELETE:
				SetKeyData(DELETE);
				break;	
			default:
				break;
		}
	}
}

/**
 * AnalysisKeyboard - 按键分析 
 */
PRIVATE void AnalysisKeyboard()
{
	unsigned char scanCode;
	int make;
	
	unsigned int key = 0;
	unsigned int *keyrow;

	keyboardPrivate.codeWithE0 = 0;

	scanCode = GetByteFromBuf();
	
	/* 检查是否是0xe1打头的数据 */
	if(scanCode == 0xe1){
		int i;
		unsigned char pausebrk_scode[] = {0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5};
		int is_pausebreak = 1;
		for(i = 1; i < 6; i++){
			if (GetByteFromBuf() != pausebrk_scode[i]) {
				is_pausebreak = 0;
				break;
			}
		}
		if (is_pausebreak) {
			key = PAUSEBREAK;
		}
	} else if(scanCode == 0xe0){
		/* 检查是否是0xe0打头的数据 */
		scanCode = GetByteFromBuf();

		//PrintScreen 被按下
		if (scanCode == 0x2A) {
			if (GetByteFromBuf() == 0xE0) {
				if (GetByteFromBuf() == 0x37) {
					key = PRINTSCREEN;
					make = 1;
				}
			}
		}
		//PrintScreen 被释放
		if (scanCode == 0xB7) {
			if (GetByteFromBuf() == 0xE0) {
				if (GetByteFromBuf() == 0xAA) {
					key = PRINTSCREEN;
					make = 0;
				}
			}
		}
		//不是PrintScreen, 此时scanCode为0xE0紧跟的那个值. 
		if (key == 0) {
			keyboardPrivate.codeWithE0 = 1;
		}
	}if ((key != PAUSEBREAK) && (key != PRINTSCREEN)) {
		/* 处理一般字符 */
		make = (scanCode & FLAG_BREAK ? 0 : 1);

		//先定位到 keymap 中的行 
		keyrow = &keymap[(scanCode & 0x7F) * MAP_COLS];
		
		keyboardPrivate.column = 0;
		int caps = keyboardPrivate.shiftLeft || keyboardPrivate.shiftRight;
		if (keyboardPrivate.capsLock) {
			if ((keyrow[0] >= 'a') && (keyrow[0] <= 'z')){
				caps = !caps;
			}
		}
		if (caps) {
			keyboardPrivate.column = 1;
		}

		if (keyboardPrivate.codeWithE0) {
			keyboardPrivate.column = 2;
		}
		
		key = keyrow[keyboardPrivate.column];
		
		switch(key) {
		case SHIFT_L:
			keyboardPrivate.shiftLeft = make;
			break;
		case SHIFT_R:
			keyboardPrivate.shiftRight = make;
			break;
		case CTRL_L:
			keyboardPrivate.ctrlLeft = make;
			break;
		case CTRL_R:
			keyboardPrivate.ctrlRight = make;
			break;
		case ALT_L:
			keyboardPrivate.altLeft = make;
			break;
		case ALT_R:
			keyboardPrivate.altLeft = make;
			break;
		case CAPS_LOCK:
			if (make) {
				keyboardPrivate.capsLock   = !keyboardPrivate.capsLock;
				SetLeds();
			}
			break;
		case NUM_LOCK:
			if (make) {
				keyboardPrivate.numLock    = !keyboardPrivate.numLock;
				SetLeds();
			}
			break;
		case SCROLL_LOCK:
			if (make) {
				keyboardPrivate.scrollLock = !keyboardPrivate.scrollLock;
				SetLeds();
			}
			break;	
		default:
			break;
		}
		
		if (make) { //忽略 Break Code
			int pad = 0;

			//首先处理小键盘
			if ((key >= PAD_SLASH) && (key <= PAD_9)) {
				pad = 1;
				switch(key) {
				case PAD_SLASH:
					key = '/';
					break;
				case PAD_STAR:
					key = '*';
					break;
				case PAD_MINUS:
					key = '-';
					break;
				case PAD_PLUS:
					key = '+';
					break;
				case PAD_ENTER:
					key = ENTER;
					break;
				default:
					if (keyboardPrivate.numLock &&
						(key >= PAD_0) &&
						(key <= PAD_9)) 
					{
						key = key - PAD_0 + '0';
					}else if (keyboardPrivate.numLock &&
						(key == PAD_DOT)) 
					{
						key = '.';
					}else{
						switch(key) {
						case PAD_HOME:
							key = HOME;
							
							break;
						case PAD_END:
							key = END;
							
							break;
						case PAD_PAGEUP:
							key = PAGEUP;
							
							break;
						case PAD_PAGEDOWN:
							key = PAGEDOWN;
							
							break;
						case PAD_INS:
							key = INSERT;
							break;
						case PAD_UP:
							key = UP;
							break;
						case PAD_DOWN:
							key = DOWN;
							break;
						case PAD_LEFT:
							key = LEFT;
							break;
						case PAD_RIGHT:
							key = RIGHT;
							break;
						case PAD_DOT:
							key = DELETE;
							break;
						default:
							break;
						}
					}
					break;
				}
			}
			
			key |= keyboardPrivate.shiftLeft	? FLAG_SHIFT_L	: 0;
			key |= keyboardPrivate.shiftRight	? FLAG_SHIFT_R	: 0;
			key |= keyboardPrivate.ctrlLeft	? FLAG_CTRL_L	: 0;
			key |= keyboardPrivate.ctrlRight	? FLAG_CTRL_R	: 0;
			key |= keyboardPrivate.altLeft	? FLAG_ALT_L	: 0;
			key |= keyboardPrivate.altRight	? FLAG_ALT_R	: 0;
			key |= pad      ? FLAG_PAD      : 0;
			KeyCharProcess(key);
		}else{
			/* 暂时不处理弹起按键 */

			//KeyCharProcess(0);
			
		}
	}
}

/**
 * TaskAssistHandler - 任务协助函数
 * @data: 传递的数据
 */
PRIVATE void TaskAssistHandler(unsigned int data)
{
	/* 先解析键值 */
	AnalysisKeyboard();
	
	/* 获取解析后的键值 */
	/*unsigned int keyData = GetKeyData();
	if (keyData != 0) {
		printk("%c", keyData);
		// 应该在这里把按键值传递给其他地方。

	}*/
}

/* 键盘任务协助 */
PRIVATE struct TaskAssist keyboardAssist;

/**
 * KeyboardHandler - 时钟中断处理函数
 * @irq: 中断号
 * @data: 中断的数据
 */
PRIVATE void KeyboardHandler(unsigned int irq, unsigned int data)
{
	/* 先从硬件获取按键数据 */
	keyboardPrivate.rowData = In8(PORT_KEYDAT);

	//printk("<%x>", keyboardPrivate.rowData);

	/* 调度任务协助 */
	TaskAssistSchedule(&keyboardAssist);
}

/**
 * KeyboardGetc - 获取字符
 * @device: 设备项
 * 
 * 获取字符有2中模式
 * 1.同步：获取字符，有数据就返回数据，不然就休眠
 * 2.异步：获取字符，有数据就返回数据，没有数据就直接诶返回
 * 
 * 成功返回键码，失败返回0（KEYCODE_NONE）
 */
PRIVATE int KeyboardGetc(struct Device *device)
{
	struct Task *current = CurrentTask();
	int ret = KEYCODE_NONE;
	int keyData;
	if (keyboardPrivate.workMode == KEYBOARD_MODE_SYNC) {
		/* 查看是否已经有数据了 */
		keyData = GetKeyData();
		/* 有效的数据 */
		if (keyData == KEYCODE_NONE) {
			/* 添加到等待队列 */
			WaitQueueAdd(&keyboardPrivate.callerWaitQueue, current);
			/* 设置为阻塞状态 */
			current->status = TASK_BLOCKED;
			/* 调度任务，让自己休眠 */
			Schedule();
			
			/* 唤醒后获取一个数据并返回 */
			keyData = GetKeyData();
		}
		ret = keyData;
	} else if (keyboardPrivate.workMode == KEYBOARD_MODE_ASYNC) {
		/* 直接获取并返回 */
		keyData = GetKeyData();
		ret = keyData;
	}

	return ret;
}

/**
 * KeyboardIoctl - 键盘的IO控制
 * @device: 设备项
 * @cmd: 命令
 * @arg1: 参数1
 * @arg2: 参数2
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int KeyboardIoctl(struct Device *device, int cmd, int arg)
{
	int retval = 0;
	switch (cmd)
	{
	case KEYBOARD_CMD_MODE:
		/* 如果是模式命令就设置模式 */
		if (arg == KEYBOARD_MODE_SYNC || arg == KEYBOARD_MODE_ASYNC)
			keyboardPrivate.workMode = arg;
		else 
			retval = -1;	/* 失败 */
		break;
		
	default:
		/* 失败 */
		retval = -1;
		break;
	}

	return retval;
}

PRIVATE struct DeviceOperations keyboardOpSets = {
	.getc = KeyboardGetc, 
	.ioctl = KeyboardIoctl, 
};

/**
 * InitKeyboardDriver - 初始化键盘驱动
 */
PUBLIC int InitKeyboardDriver()
{
	/* 分配一个字符设备 */
	keyboardPrivate.chrdev = AllocCharDevice(DEV_KEYBOARD);
	if (keyboardPrivate.chrdev == NULL) {
		printk(PART_ERROR "alloc char device for keyboard failed!\n");
		return -1;
	}

	/* 初始化字符设备信息 */
	CharDeviceInit(keyboardPrivate.chrdev, 1, &keyboardPrivate);
	CharDeviceSetup(keyboardPrivate.chrdev, &keyboardOpSets);

	CharDeviceSetName(keyboardPrivate.chrdev, DEVNAME);
	
	/* 把字符设备添加到系统 */
	AddCharDevice(keyboardPrivate.chrdev);
	
	/* 初始化私有数据 */
	keyboardPrivate.codeWithE0 = 0;
	
	keyboardPrivate.shiftLeft	= keyboardPrivate.shiftRight = 0;
	keyboardPrivate.altLeft	= keyboardPrivate.altRight   = 0;
	keyboardPrivate.ctrlLeft	= keyboardPrivate.ctrlRight  = 0;
	
	keyboardPrivate.capsLock   = 0;
	keyboardPrivate.numLock    = 1;
	keyboardPrivate.scrollLock = 0;
	
	keyboardPrivate.keyData = 0;
	keyboardPrivate.rowData = 0;

	keyboardPrivate.workMode = KEYBOARD_MODE_SYNC;
	WaitQueueInit(&keyboardPrivate.callerWaitQueue, NULL);

	/* 初始化键盘控制器 */
	KeyboardControllerWait();
	Out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	KeyboardControllerAck();

	KeyboardControllerWait();
	Out8(PORT_KEYDAT, KBC_MODE);
	KeyboardControllerAck();

	/* 注册时钟中断并打开中断 */	
	RegisterIRQ(IRQ1_KEYBOARD, KeyboardHandler, IRQF_DISABLED, "IRQ1", DEVNAME, 0);

	/* 初始化任务协助 */
	TaskAssistInit(&keyboardAssist, TaskAssistHandler, 0);

	return 0;
}

/**
 * ExitKeyboardDriver - 退出键盘驱动
 */
PUBLIC void ExitKeyboardDriver()
{
	/* 关闭设备后注销中断 */
	UnregisterIRQ(IRQ1, 0);
	DelCharDevice(keyboardPrivate.chrdev);
	FreeCharDevice(keyboardPrivate.chrdev);
}
