/*
 * file:		drivers/tty.c
 * auther:		Jason Hu
 * time:		2019/8/8
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <book/arch.h>
#include <book/debug.h>
#include <drivers/tty.h>
#include <book/device.h>
#include <share/string.h>
#include <share/vsprintf.h>

#define DEVNAME "tty"

/* TeleType 终端 */
typedef struct TTY {
    struct IoQueue ioqueue;     /* 输入输出队列，存放数据 */
    dev_t conDevno;             /* 对应的控制台的设备号 */
    struct CharDevice *chrdev;  /* 字符设备 */
    int key;                    /* 设备按键 */
} TTY_t;

/* 根据控制台数量创建tty数量 */
TTY_t ttyTable[MAX_CONSOLE_NR];

#define TTY_FIRST	(ttyTable)
#define TTY_END		(ttyTable + MAX_CONSOLE_NR)

#define IS_CURRENT_TTY(tty) \
        tty == currentTTY

#define IS_CORRECT_TTYID(ttyid) \
        (ttyid >= 0 && ttyid < MAX_CONSOLE_NR)

PRIVATE void TTY_DoRead(TTY_t *tty);
PRIVATE void TTY_DoWrite(TTY_t *tty);
PRIVATE void TTY_PutKey(TTY_t *tty, u32 key);

PRIVATE TTY_t *currentTTY;

/**
 * SelectConsole - 选择控制台
 * @consoleID: 控制台id
 * 
 * id范围[0 ~ (MAX_CONSOLE_NR - 1)]
 */
PRIVATE void SelectTTY(int ttyID)
{
	if ((ttyID < 0) || (ttyID >= MAX_CONSOLE_NR)) {
		return;
	}
    currentTTY = ttyTable + ttyID;
    /* 刷新成当前控制台的光标位置 */

	DeviceIoctl(currentTTY->conDevno, CON_CMD_SELECT_CON, ttyID);
}

/**
 * KeyCharProcess - 对单独的按键进行处理
 * @key: 按键 
 * 
 * 可以在这里面处理一些系统的按键
 */
PUBLIC void TTY_ProcessKey(TTY_t *tty, unsigned int key)
{
	/* 没有扩展数据，就直接写入对应的key */
	if (!(key & FLAG_EXT)) {
		
		TTY_PutKey(tty, key);
	} else {
		/* 有扩展的数据，需要解析 */
		int raw_code = key & MASK_RAW;
		
		switch(raw_code) {
			case ENTER:
				TTY_PutKey(tty, '\n');
				break;
			case BACKSPACE:
				TTY_PutKey(tty,  '\b');
				break;
			case TAB:
				
				break;
			case F1:
				//TTY_PutKey(tty,  F1);
				if(key & FLAG_ALT_L || key & FLAG_ALT_R){
                    SelectTTY(0);
					//DeviceIoctl(tty->conDevno, CON_CMD_SELECT_CON, 0);
				}
				break;
			case F2:
				//TTY_PutKey(tty,  F2);
				if(key & FLAG_ALT_L || key & FLAG_ALT_R){
                    SelectTTY(1);
                    //DeviceIoctl(tty->conDevno, CON_CMD_SELECT_CON, 1);
				}
				break;
				
			case F3:
				//shft +f4 关闭窗口程序
				//TTY_PutKey(tty,  F3);
				if(key & FLAG_ALT_L || key & FLAG_ALT_R){
                    SelectTTY(2);
					//DeviceIoctl(tty->conDevno, CON_CMD_SELECT_CON, 2);
				}
				break;
			case F4:
				//alt +f4 关闭程序
				TTY_PutKey(tty,  F4);
                if(key & FLAG_ALT_L || key & FLAG_ALT_R){
                    SelectTTY(3);
					//DeviceIoctl(tty->conDevno, CON_CMD_SELECT_CON, 3);
				}
				break;	
			case F5:
				TTY_PutKey(tty,  F5);

				break;	  
			case F6:
				TTY_PutKey(tty,  F6);
				break;	   
			case F7:
				TTY_PutKey(tty,  F7);
				break;	  
			case F8:
				TTY_PutKey(tty,  F8);
				break;	  
			case F9:
				TTY_PutKey(tty,  F9);
				break;	  
			case F10:
				TTY_PutKey(tty,  F10);
				break;	  
			case F11:
				TTY_PutKey(tty,  F11);
				break;	  
			case F12:  
				
                
                //TTY_PutKey(tty,  F12);
                
                DeviceIoctl(tty->conDevno, CON_CMD_CLEAN, 0);
				break;
			case ESC:
				TTY_PutKey(tty, ESC);
				
				break;	
			case UP:
				if (key & FLAG_CTRL_L || key & FLAG_CTRL_R) {
                    DeviceIoctl(tty->conDevno, CON_CMD_SCROLL, SCREEN_UP);
                }
                TTY_PutKey(tty, UP);
				
				break;
			case DOWN:
                if (key & FLAG_CTRL_L || key & FLAG_CTRL_R) {
                    DeviceIoctl(tty->conDevno, CON_CMD_SCROLL, SCREEN_DOWN);
                }
				TTY_PutKey(tty, DOWN);
				
				break;
			case LEFT:
				
				TTY_PutKey(tty, LEFT);
				break;
			case RIGHT:
				TTY_PutKey(tty, RIGHT);
				break;
			case PAGEUP:
				TTY_PutKey(tty, PAGEUP);
				break;
			case PAGEDOWN:
				TTY_PutKey(tty, PAGEDOWN);
				break;	
			case HOME:
				TTY_PutKey(tty, HOME);
				break;	
			case END:
				TTY_PutKey(tty, END);
				break;	
			case INSERT:
				TTY_PutKey(tty, INSERT);
				break;	
			case DELETE:
				TTY_PutKey(tty, DELETE);
				break;	
			default:
				break;
		}
	}
}

/**
 * TTY_PutKey - 往tty终端的队列中放入一个按键
 * @tty: tty终端
 * @key: 要放入的按键
 */
PRIVATE void TTY_PutKey(TTY_t *tty, u32 key)
{
    /* 把按键放入tty的io队列 */
    IoQueuePut(&tty->ioqueue, key);
    tty->key = key; /* 获取最新的key */
    //printk("key:%x", key);
}

/**
 * TTY_DoRead - tty执行读取操作
 * @tty: tty终端
 * 
 */
PRIVATE void TTY_DoRead(TTY_t *tty)
{
    /* 如果是当前控制台，才会进行键盘的读取 */
	if (IS_CURRENT_TTY(tty)) {
        /* 读取键盘数据 */
        unsigned int key = 0;
    
        /* 键盘读取采用同步机制，如果没有按键，来读取的任务就会休眠 */
        key = DeviceGetc(DEV_KEYBOARD);
        
        /* 读取成功后处理按键，不是空按键才会往tty放入数据 */
        if (key != KEYCODE_NONE) {
            TTY_ProcessKey(tty, key);
        }
	}
}

/**
 * TTY_DoWrite - tty执行写入操作
 * @tty: tty终端
 */
PRIVATE void TTY_DoWrite(TTY_t *tty)
{
    /* 队列不为空，有数据才获取 */
    if (!IoQueueEmpty(&tty->ioqueue)) {
        /* 获取一个字符 */
        unsigned int key = IoQueueGet(&tty->ioqueue);
        /* 写入控制台 */
        //DevicePutc(tty->conDevno, key);
    }
}

/**
 * TTY_Write - tty终端写入接口
 * @tty: 哪个tty
 * @buf: 缓冲区
 * @len: 写入的数据长度（字节）
 * 
 * 往tty对应的控制台写入字符数据
 */
PRIVATE int TTY_Getc(struct Device *device)
{
    /* 获取控制台 */
    struct CharDevice *chrdev = (struct CharDevice *)device;
    TTY_t *tty = (TTY_t *)chrdev->private;
    
    int key = KEYCODE_NONE;
    /* 如果是当前控制台，才会进行键盘的读取 */
	if (IS_CURRENT_TTY(tty)) {
        //printk("getc");
        /*if (!IoQueueEmpty(&tty->ioqueue)) {
            key = IoQueueGet(&tty->ioqueue);
        }*/
        key = tty->key;
        tty->key = KEYCODE_NONE;
    }
    return key;
}

/**
 * TTY_Write - tty终端写入接口
 * @tty: 哪个tty
 * @buf: 缓冲区
 * @len: 写入的数据长度（字节）
 * 
 * 往tty对应的控制台写入字符数据
 * 由于Putc的效率可能比较低，所以采用write
 */
PRIVATE int TTY_Write(struct Device *device, unsigned int off, void *buffer, unsigned int len)
{
    /* 获取控制台 */
    struct CharDevice *chrdev = (struct CharDevice *)device;
    TTY_t *tty = (TTY_t *)chrdev->private;
    
    char *p = (char *)buffer;
    int i = len;
    while (i) {
        /* 输出字符到控制台 */
        DevicePutc(tty->conDevno, *p++);
        i--;
    }
    
    return 0;
}

/**
 * TTY_Write - tty终端写入接口
 * @tty: 哪个tty
 * @buf: 缓冲区
 * @len: 写入的数据长度（字节）
 * 
 * 往tty对应的控制台写入字符数据
 */
PRIVATE int TTY_Putc(struct Device *device, unsigned int ch)
{
    /* 获取控制台 */
    struct CharDevice *chrdev = (struct CharDevice *)device;
    TTY_t *tty = (TTY_t *)chrdev->private;
    
    /* 输出字符到控制台 */
    DevicePutc(tty->conDevno, ch);

    return 0;
}

/**
 * TTY_Ioctl - tty的IO控制
 * @device: 设备
 * @cmd: 命令
 * @arg: 参数
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int TTY_Ioctl(struct Device *device, int cmd, int arg)
{
/* 获取控制台 */
    struct CharDevice *chrdev = (struct CharDevice *)device;
    TTY_t *tty = (TTY_t *)chrdev->private;
    
	int retval = 0;
	switch (cmd)
	{
    case TTY_CMD_CLEAN:
        DeviceIoctl(tty->conDevno, CON_CMD_CLEAN, 0);
        break;
	default:
		/* 失败 */
		retval = -1;
		break;
	}

	return retval;
}


PRIVATE struct DeviceOperations ttyOpSets = {
	.write = TTY_Write,
    .getc = TTY_Getc,
    .putc = TTY_Putc,
    .ioctl = TTY_Ioctl,
};

PRIVATE int TTY_InitOne(TTY_t *tty)
{
    int id = tty - ttyTable;
    /* 添加字符设备 */

    /* 设置一个字符设备号 */
    tty->chrdev = AllocCharDevice(MKDEV(TTY_MAJOR, id));
	if (tty->chrdev == NULL) {
		printk(PART_ERROR "alloc char device for tty failed!\n");
		return -1;
	}

	/* 初始化字符设备信息 */
	CharDeviceInit(tty->chrdev, 1, tty);
	CharDeviceSetup(tty->chrdev, &ttyOpSets);

    char devname[DEVICE_NAME_LEN] = {0};
    sprintf(devname, "%s%d", DEVNAME, id);
	CharDeviceSetName(tty->chrdev, devname);
	
	/* 把字符设备添加到系统 */
	AddCharDevice(tty->chrdev);

    unsigned char *buf = kmalloc(IQ_BUF_LEN_8, GFP_KERNEL);
    if (buf == NULL)
        return -1;
    /* 初始化io队列 */
    IoQueueInit(&tty->ioqueue, buf, IQ_BUF_LEN_8, IQ_FACTOR_8);
    
    /* 生成控制台设备号 */
    tty->conDevno = MKDEV(CONSOLE_MAJOR, id);

    /* 打开控制台设备 */
    if (DeviceOpen(tty->conDevno, 0)) {
        printk("open console device failed!\n");
        return -1;
    }
    return 0;
}

/**
 * TaskTTY - tty任务
 * @arg: 参数
 */
PUBLIC void TaskTTY(void *arg)
{
    /* 记得在开启tty任务之前初始化键盘 */
	TTY_t *tty;
    /* 打开键盘设备 */
    DeviceOpen(DEV_KEYBOARD, 0);

    /* 初始化每一个tty */
	for (tty = TTY_FIRST; tty < TTY_END; tty++) {
		TTY_InitOne(tty);
	}

    /* 选择第一个TTY */
    SelectTTY(0);

	while (1) {
        /* 轮询全部tty，对每个tty进行读取和写入操作，只有是当前tty才会真正操作 */
		for (tty = TTY_FIRST; tty < TTY_END; tty++) {
			TTY_DoRead(tty);
			TTY_DoWrite(tty);
		}
	}
}
