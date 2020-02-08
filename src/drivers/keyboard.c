/*
 * file:		drivers/char/keyboard.c
 * auther:		Jason Hu
 * time:		2019/8/19
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/interrupt.h>
#include <drivers/keyboard.h>
#include <drivers/keymap.h>
#include <book/ioqueue.h>
#include <book/device.h>
#include <share/string.h>
#include <book/chr-dev.h>
#include <drivers/console.h>
#include <book/ioqueue.h>
#include <drivers/tty.h>

#define DRV_NAME "keyboard"
#define DRV_VERSION "0.1"

/* 把小键盘上的数值修复成主键盘上的值 */
//#define CONFIG_PAD_FIX

/* 键盘控制器端口 */
enum KeyboardControllerPorts {
    KBC_READ_DATA   = 0x60,     /* 读取数据端口(R) */
    KBC_WRITE_DATA  = 0x60,     /* 写入数据端口(W) */
    KBC_STATUS      = 0x64,     /* 获取控制器状态(R) */
    KBC_CMD         = 0x64,     /* 向控制器发送命令(W) */
};

/* 键盘控制器的命令 */
enum KeyboardControllerCmds {
    KBC_CMD_READ_CONFIG     = 0x20,     /* 读取配置命令 */
    KBC_CMD_WRITE_CONFIG    = 0x60,     /* 写入配置命令 */
    KBC_CMD_DISABLE_MOUSE   = 0xA7,     /* 禁止鼠标端口 */
    KBC_CMD_ENABLE_MOUSE    = 0xA8,     /* 开启鼠标端口 */
    KBC_CMD_DISABLE_KEY     = 0xAD,     /* 禁止键盘通信，自动复位1控制器状态的第4位 */
    KBC_CMD_ENABLE_KEY      = 0xAE,     /* 开启键盘通信，自动置位0控制器状态的第4位 */
    KBC_CMD_SEND_TO_MOUSE   = 0xD4,     /* 向鼠标发送数据 */
    KBC_CMD_REBOOT_SYSTEM   = 0xFE,     /* 系统重启 */    
};

/* 键盘配置位 */
enum KeyboardControllerConfigBits {
    KBC_CFG_ENABLE_KEY_INTR     = (1 << 0), /* bit 0=1: 使能键盘中断IRQ1(IBE) */
    KBC_CFG_ENABLE_MOUSE_INTR   = (1 << 1), /* bit 1=1: 使能鼠标中断IRQ12(MIBE) */
    KBC_CFG_INIT_DONE           = (1 << 2), /* bit 2=1: 设置状态寄存器的位2 */
    KBC_CFG_IGNORE_STATUS_BIT4  = (1 << 3), /* bit 3=1: 忽略状态寄存器中的位4 */
    KBC_CFG_DISABLE_KEY         = (1 << 4), /* bit 4=1: 禁止键盘 */
    KBC_CFG_DISABLE_MOUSE       = (1 << 5), /* bit 5=1: 禁止鼠标 */
    KBC_CFG_SCAN_CODE_TRANS     = (1 << 6), /* bit 6=1: 将第二套扫描码翻译为第一套 */
    /* bit 7 保留为0 */
};

/* 键盘控制器状态位 */
enum KeyboardControllerStatusBits {
    KBC_STATUS_OUT_BUF_FULL     = (1 << 0), /* OUT_BUF_FULL: 输出缓冲器满置1，CPU读取后置0 */
    KBC_STATUS_INPUT_BUF_FULL   = (1 << 1), /* INPUT_BUF_FULL: 输入缓冲器满置1，i8042 取走后置0 */
    KBC_STATUS_SYS_FLAG         = (1 << 2), /* SYS_FLAG: 系统标志，加电启动置0，自检通过后置1 */
    KBC_STATUS_CMD_DATA         = (1 << 3), /* CMD_DATA: 为1，输入缓冲器中的内容为命令，为0，输入缓冲器中的内容为数据。 */
    KBC_STATUS_KYBD_INH         = (1 << 4), /* KYBD_INH: 为1，键盘没有被禁止。为0，键盘被禁止。 */
    KBC_STATUS_TRANS_TMOUT      = (1 << 5), /* TRANS_TMOUT: 发送超时，置1 */
    KBC_STATUS_RCV_TMOUT        = (1 << 6), /* RCV-TMOUT: 接收超时，置1 */
    KBC_STATUS_PARITY_EVEN      = (1 << 7), /* PARITY-EVEN: 从键盘获得的数据奇偶校验错误 */
};

/* 键盘控制器发送命令后 */
enum KeyboardControllerReturnCode {
    /* 当击键或释放键时检测到错误时，则在Output Bufer后放入此字节，
    如果Output Buffer已满，则会将Output Buffer的最后一个字节替代为此字节。
    使用Scan code set 1时使用00h，Scan code 2和Scan Code 3使用FFh。 */
    KBC_RET_KEY_ERROR_00    = 0x00,
    KBC_RET_KEY_ERROR_FF    = 0xFF,
    
    /* AAH, BAT完成代码。如果键盘检测成功，则会将此字节发送到8042 Output Register中。 */
    KBC_RET_BAT_OK          = 0xAA,

    /* EEH, Echo响应。Keyboard使用EEh响应从60h发来的Echo请求。 */
    KBC_RET_ECHO            = 0xEE,

    /* F0H, 在Scan code set 2和Scan code set 3中，被用作Break Code的前缀。*/
    KBC_RET_BREAK           = 0xF0,
    /* FAH, ACK。当Keyboard任何时候收到一个来自于60h端口的合法命令或合法数据之后，
    都回复一个FAh。 */
    KBC_RET_ACK             = 0xFA,
    
    /* FCH, BAT失败代码。如果键盘检测失败，则会将此字节发送到8042 Output Register中。 */
    KBC_RET_BAT_BAD         = 0xFC,

    /* FEH, 当Keyboard任何时候收到一个来自于60h端口的非法命令或非法数据之后，
    或者数据的奇偶交验错误，都回复一个FEh，要求系统重新发送相关命令或数据。 */
    KBC_RET_RESEND          = 0xFE,
};

/* 单独发送给键盘的命令，有别于键盘控制器命令
这些命令是发送到数据端口0x60，而不是命令0x64，
如果有参数，就在发送一次到0x60即可。
 */
enum KeyboardCmds {
    /* LED灯亮/灭，参数如下：
        位2：Caps Lock灯 1（亮）/0（灭）
        位1：Num Lock灯 1（亮）/0（灭）
        位0：Scroll Lock灯 1（亮）/0（灭）
     */
    KEY_CMD_LED_CODE        =  0xED,    /* 控制LED灯 */   
    
    /* 扫码集的参数：
        0x01： 取得当前扫描码（有返回值）
        0x02： 代表第一套扫描码
        0x03： 代表第二套扫描码
        0x04： 代表第三套扫描码
     */
    KEY_CMD_SET_SCAN_CODE   =  0xF0,    /* 设置键盘使用的扫码集*/        
    KEY_CMD_GET_DEVICE_ID   =  0xF2,    /* 获取键盘设备的ID号（2B） */                
    KEY_CMD_START_SCAN      =  0xF4,    /* 开启键盘扫描 */
    KEY_CMD_STOP_SCAN       =  0xF5,    /* 停止键盘扫描 */
    KEY_CMD_RESTART         =  0xFF,    /* 重启键盘 */
};

/* 键盘控制器配置 */
#define KBC_CONFIG	(KBC_CFG_ENABLE_KEY_INTR | KBC_CFG_ENABLE_MOUSE_INTR | \
                    KBC_CFG_INIT_DONE | KBC_CFG_SCAN_CODE_TRANS)

/* 等待键盘控制器可写入，当输入缓冲区为空后才可以写入 */
#define WAIT_KBC_WRITE()    while (In8(KBC_STATUS) & KBC_STATUS_INPUT_BUF_FULL)
/* 等待键盘控制器可读取，当输出缓冲区为空后才可以读取 */
#define WAIT_KBC_READ()    while (In8(KBC_STATUS) & KBC_STATUS_OUT_BUF_FULL)

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
	
    struct IoQueue ioqueue; /* io队列 */
	struct CharDevice *chrdev;	/* 字符设备 */
};

/* 键盘的私有数据 */
PRIVATE struct KeyboardPrivate keyboardPrivate;

/* 等待键盘控制器应答，如果不是回复码就一直等待
这个本应该是宏的，但是在vmware虚拟机中会卡在那儿，所以改成宏类函数
 */
PRIVATE void WAIT_KBC_ACK()
{
	unsigned char read;
	do {
		read = In8(KBC_READ_DATA);
	} while ((read =! KBC_RET_ACK));
}

/**
 * SetLeds - 设置键盘led灯状态
 */
PRIVATE void SetLeds()
{
	/* 先合成成为一个数据，后面写入寄存器 */
	unsigned char leds = (keyboardPrivate.capsLock << 2) | 
        (keyboardPrivate.numLock << 1) | keyboardPrivate.scrollLock;
	
	/* 数据指向led */
	WAIT_KBC_WRITE();
	Out8(KBC_WRITE_DATA, KEY_CMD_LED_CODE);
	WAIT_KBC_ACK();
	/* 写入新的led值 */
	WAIT_KBC_WRITE();
	Out8(KBC_WRITE_DATA, leds);
    WAIT_KBC_ACK();
}

/**
 * GetByteFromBuf - 从键盘缓冲区中读取下一个字节
 */
PRIVATE unsigned char GetByteFromBuf()       
{
    unsigned char scanCode;

    /* 从队列中获取一个数据 */
    scanCode = IoQueueGet(&keyboardPrivate.ioqueue);
    
    return scanCode;
}

/**
 * AnalysisKeyboard - 按键分析 
 */
PUBLIC unsigned int KeyboardDoRead()
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
			key = KBD_PAUSEBREAK;
		}
	} else if(scanCode == 0xe0){
		/* 检查是否是0xe0打头的数据 */
		scanCode = GetByteFromBuf();

		//PrintScreen 被按下
		if (scanCode == 0x2A) {
			if (GetByteFromBuf() == 0xE0) {
				if (GetByteFromBuf() == 0x37) {
					key = KBD_PRINTSCREEN;
					make = 1;
				}
			}
		}
		//PrintScreen 被释放
		if (scanCode == 0xB7) {
			if (GetByteFromBuf() == 0xE0) {
				if (GetByteFromBuf() == 0xAA) {
					key = KBD_PRINTSCREEN;
					make = 0;
				}
			}
		}
		//不是PrintScreen, 此时scanCode为0xE0紧跟的那个值. 
		if (key == 0) {
			keyboardPrivate.codeWithE0 = 1;
		}
	}if ((key != KBD_PAUSEBREAK) && (key != KBD_PRINTSCREEN)) {
		/* 处理一般字符 */
		make = (scanCode & KBD_FLAG_BREAK ? 0 : 1);

		//先定位到 keymap 中的行 
		keyrow = &keymap[(scanCode & 0x7F) * KEYMAP_COLS];
		
		keyboardPrivate.column = 0;
		int caps = keyboardPrivate.shiftLeft || keyboardPrivate.shiftRight;
		if (keyboardPrivate.capsLock) {
			if ((keyrow[0] >= 'a') && (keyrow[0] <= 'z')){
				caps = !caps;
			}
		}
        /* 如果大写打开 */
		if (caps) {
			keyboardPrivate.column = 1;
		}

        /* 如果有0xE0数据 */
		if (keyboardPrivate.codeWithE0) {
			keyboardPrivate.column = 2;
		}
		/* 读取列中的数据 */
		key = keyrow[keyboardPrivate.column];
		
        /* shift，ctl，alt变量设置，
        caps，num，scroll锁设置 */
		switch(key) {
		case KBD_SHIFT_L:
			keyboardPrivate.shiftLeft = make;
			break;
		case KBD_SHIFT_R:
			keyboardPrivate.shiftRight = make;
			break;
		case KBD_CTRL_L:
			keyboardPrivate.ctrlLeft = make;
			break;
		case KBD_CTRL_R:
			keyboardPrivate.ctrlRight = make;
			break;
		case KBD_ALT_L:
			keyboardPrivate.altLeft = make;
			break;
		case KBD_ALT_R:
			keyboardPrivate.altLeft = make;
			break;
		case KBD_CAPS_LOCK:
			if (make) {
				keyboardPrivate.capsLock   = !keyboardPrivate.capsLock;
				SetLeds();
			}
			break;
		case KBD_NUM_LOCK:
			if (make) {
				keyboardPrivate.numLock    = !keyboardPrivate.numLock;
				SetLeds();
			}
			break;
		case KBD_SCROLL_LOCK:
			if (make) {
				keyboardPrivate.scrollLock = !keyboardPrivate.scrollLock;
				SetLeds();
			}
			break;	
		default:
			break;
		}
        int pad = 0;
        //首先处理小键盘
        if ((key >= KBD_PAD_SLASH) && (key <= KBD_PAD_9)) {
            pad = 1;
#ifdef CONFIG_PAD_FIX
            switch(key) {
            case KBD_PAD_SLASH:
                key = '/';
                break;
            case KBD_PAD_STAR:
                key = '*';
                break;
            case KBD_PAD_MINUS:
                key = '-';
                break;
            case KBD_PAD_PLUS:
                key = '+';
                break;
            case KBD_PAD_ENTER:
                key = KBC_ENTER;
                break;
            default:
                if (keyboardPrivate.numLock &&
                    (key >= KBD_PAD_0) &&
                    (key <= KBD_PAD_9)) 
                {
                    key = key - KBD_PAD_0 + '0';
                }else if (keyboardPrivate.numLock &&
                    (key == KBD_PAD_DOT)) 
                { 
                    key = '.';
                }else{
                    switch(key) {
                    case KBD_PAD_HOME:
                        key = KBC_HOME;
                        
                        break;
                    case KBD_PAD_END:
                        key = KBC_END;
                        
                        break;
                    case KBD_PAD_PAGEUP:
                        key = KBC_PAGEUP;
                        
                        break;
                    case KBD_PAD_PAGEDOWN:
                        key = KBC_PAGEDOWN;
                        
                        break;
                    case KBD_PAD_INS:
                        key = KBC_INSERT;
                        break;
                    case KBD_PAD_UP:
                        key = KBC_UP;
                        break;
                    case KBD_PAD_DOWN:
                        key = KBC_DOWN;
                        break;
                    case KBD_PAD_LEFT:
                        key = KBC_LEFT;
                        break;
                    case KBD_PAD_RIGHT:
                        key = KBC_RIGHT;
                        break;
                    case KBD_PAD_DOT:
                        key = KBC_DELETE;
                        break;
                    default:
                        break;
                    }
                }
                break;
            }
#endif /* CONFIG_PAD */
        }
        
        /* 如果有组合件，就需要合成成为组合后的按钮，可以是ctl+alt+shift+按键的格式 */
        key |= keyboardPrivate.shiftLeft	? KBD_FLAG_SHIFT_L	: 0;
        key |= keyboardPrivate.shiftRight	? KBD_FLAG_SHIFT_R	: 0;
        key |= keyboardPrivate.ctrlLeft	? KBD_FLAG_CTRL_L	: 0;
        key |= keyboardPrivate.ctrlRight	? KBD_FLAG_CTRL_R	: 0;
        key |= keyboardPrivate.altLeft	? KBD_FLAG_ALT_L	: 0;
        key |= keyboardPrivate.altRight	? KBD_FLAG_ALT_R	: 0;
        key |= pad      ? KBD_FLAG_PAD      : 0;

        /* 如果是BREAK,就需要添加BREAK标志 */
        key |= make ? 0: KBD_FLAG_BREAK;
        
        /* 设置锁标志 */
        key |= keyboardPrivate.numLock ? KBD_FLAG_NUM : 0;
        key |= keyboardPrivate.capsLock ? KBD_FLAG_CAPS : 0;

        /* 把按键输出 */
        return key;
	}
    return KEYCODE_NONE;
}

/**
 * KeyboardHandler - 时钟中断处理函数
 * @irq: 中断号
 * @data: 中断的数据
 */
PRIVATE void KeyboardHandler(unsigned int irq, unsigned int data)
{
	/* 先从硬件获取按键数据 */
	unsigned char scanCode = In8(KBC_READ_DATA);

    /* 把数据放到io队列 */
    IoQueuePut(&keyboardPrivate.ioqueue, scanCode);
}


/**
 * KeyboardRead - 从键盘读取
 * @device: 设备
 * @unused: 未用
 * @buffer: 存放数据的缓冲区
 * @len: 数据长度
 * 
 * 从键盘读取输入, 根据len读取不同的长度
 */
PRIVATE int KeyboardRead(struct Device *device, unsigned int unused, void *buffer, unsigned int len)
{
    if (len == 1) {
        *(char *)buffer = (char )KeyboardDoRead();
    } else if (len == 2) {
        *(short *)buffer = (short )KeyboardDoRead();
    } else if (len == 4) {
        *(unsigned int *)buffer = (unsigned int )KeyboardDoRead();
    }
    return 0;
}

/**
 * KeyboardGetc - 从键盘获取一个按键
 * @device: 设备
 */
PRIVATE int KeyboardGetc(struct Device *device)
{
    return KeyboardDoRead();    /* 读取键盘 */
}

/**
 * KeyboardIoctl - 键盘的IO控制
 * @device: 设备项
 * @cmd: 命令
 * @arg: 参数
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int KeyboardIoctl(struct Device *device, int cmd, int arg)
{
	int retval = 0;
	switch (cmd)
	{
	
	default:
		/* 失败 */
		retval = -1;
		break;
	}

	return retval;
}

PRIVATE struct DeviceOperations keyboardOpSets = {
	.ioctl = KeyboardIoctl, 
    .getc = KeyboardGetc,
    .read = KeyboardRead,
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

	CharDeviceSetName(keyboardPrivate.chrdev, DRV_NAME);
	
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

	unsigned char *buf = kmalloc(IQ_BUF_LEN_32, GFP_KERNEL);
    if (buf == NULL)
        return -1;
    /* 初始化io队列 */
    IoQueueInit(&keyboardPrivate.ioqueue, buf, IQ_BUF_LEN_32, IQ_FACTOR_32);

    
	/* 注册时钟中断并打开中断，因为设定硬件过程中可能产生中断，所以要提前打开 */	
	RegisterIRQ(IRQ1_KEYBOARD, KeyboardHandler, IRQF_DISABLED, "IRQ1", DRV_NAME, (uint32_t)&keyboardPrivate);
    
    /* 初始化键盘控制器 */

    /* 发送写配置命令 */
    WAIT_KBC_WRITE();
	Out8(KBC_CMD, KBC_CMD_WRITE_CONFIG);
    WAIT_KBC_ACK();
    
    /* 往数据端口写入配置值 */
    WAIT_KBC_WRITE();
	Out8(KBC_WRITE_DATA, KBC_CONFIG);
	WAIT_KBC_ACK();

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
