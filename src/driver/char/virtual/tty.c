/*
 * file:		char/virtual/tty.c
 * auther:		Jason Hu
 * time:		2019/8/8
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/debug.h>
#include <book/device.h>
#include <book/signal.h>
#include <lib/string.h>
#include <lib/vsprintf.h>
#include <lib/stddef.h>
#include <char/virtual/tty.h>
#include <char/console/console.h>
#include <input/keyboard/ps2.h>

#define DEVNAME "tty"

/* 一般情况下，tty和console一样多
再图形模式下，把最后一个tty留给图形模式
 */
#define MAX_TTY_NR (MAX_CONSOLE_NR + 1) 

/* 图形模式下面的ttyID */
#define TTY_GRAPH_ID    MAX_TTY_NR - 1

/* TeleType 终端 */
typedef struct TTY {
    struct IoQueue ioqueue;     /* 输入输出队列，存放数据 */
    dev_t conDevno;             /* 对应的控制台的设备号 */
    struct CharDevice *chrdev;  /* 字符设备 */
    int key;                    /* 设备按键 */
    pid_t holdPid;              /* 持有者进程 */
    int deviceID;               /* 设备ID，0~MAX_TTY_NR-1 */
    
} TTY_t;

/* 根据控制台数量创建tty数量 */
TTY_t ttyTable[MAX_TTY_NR];

#define TTY_FIRST	(ttyTable)
#define TTY_END		(ttyTable + MAX_TTY_NR)

#define IS_CURRENT_TTY(tty) \
        tty == currentTTY

#define IS_CORRECT_TTYID(ttyid) \
        (ttyid >= 0 && ttyid < MAX_TTY_NR)

PRIVATE void TTY_DoRead(TTY_t *tty);
PRIVATE void TTY_DoWrite(TTY_t *tty);
PRIVATE void TTY_PutKey(TTY_t *tty, u32 key);

PRIVATE TTY_t *currentTTY;

/**
 * SelectConsole - 选择控制台
 * @consoleID: 控制台id
 * 
 * id范围[0 ~ (MAX_TTY_NR - 1)]
 */
PRIVATE void SelectTTY(int ttyID)
{
	if ((ttyID < 0) || (ttyID >= MAX_TTY_NR)) {
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
	if (!(key & KBD_FLAG_EXT)) {
        
        /* 如果是图形tty，就直接传输过去，让图形核心处理 */
        if (tty->deviceID == TTY_GRAPH_ID) {
            /* 发送给图形tty，按下和弹起都接收 */
            TTY_PutKey(tty, key);
        } else {
            /* 只获取按下键，不处理弹起 */
            if (!(key & KBD_FLAG_BREAK)) {
                /* ctrl + 字符 */
                if(key & KBD_FLAG_CTRL_L || key & KBD_FLAG_CTRL_R){
                    char ch = key & 0xff;
                    //printk("<ctr + %c>\n", ch);
                    switch (ch)
                    {
                    /* ctl + c 结束一个前台进程 */
                    case 'c':
                    case 'C':
                        if (tty->holdPid > 0) {
                            /* 发送终止提示符 */
                            DevicePutc(tty->conDevno, '^');
                            DevicePutc(tty->conDevno, 'C');
                            DevicePutc(tty->conDevno, '\n');
                            SysKill(tty->holdPid, SIGINT);
                        }
                        break;
                    /* ctl + \ 结束一个前台进程 */
                    case '\\':
                        if (tty->holdPid > 0) {
                            DevicePutc(tty->conDevno, '^');
                            DevicePutc(tty->conDevno, '\\');
                            DevicePutc(tty->conDevno, '\n');
                            SysKill(tty->holdPid, SIGQUIT);
                        }
                        break;
                    /* ctl + z 让前台进程暂停运行 */
                    case 'z':
                    case 'Z':
                        if (tty->holdPid > 0) {
                            DevicePutc(tty->conDevno, '^');
                            DevicePutc(tty->conDevno, 'Z');
                            DevicePutc(tty->conDevno, '\n');
                            SysKill(tty->holdPid, SIGTSTP);
                        }
                        break;
                    /* ctl + x 恢复前台进程运行 */
                    case 'x':
                    case 'X':
                        if (tty->holdPid > 0) {
                            DevicePutc(tty->conDevno, '^');
                            DevicePutc(tty->conDevno, 'X');
                            DevicePutc(tty->conDevno, '\n');
                            SysKill(tty->holdPid, SIGCONT);
                        }
                        break;
                    default:
                        break;
                    }
                } else {
                    TTY_PutKey(tty, key);
                }
            }
        }
	} else {
        /* 有扩展的数据，需要解析 */
		int raw_code = key & KBD_MASK_RAW;
		if (tty->deviceID == TTY_GRAPH_ID) {
            /* 图形tty下面的切换 */
            switch(raw_code) {
			case KBD_F1:
				//TTY_PutKey(tty,  F1);
				if(key & KBD_FLAG_ALT_L || key & KBD_FLAG_ALT_R){
                    SelectTTY(0);
					//DeviceIoctl(tty->conDevno, CON_CMD_SELECT_CON, 0);
				}
				break;
			case KBD_F2:
				//TTY_PutKey(tty,  F2);
				if(key & KBD_FLAG_ALT_L || key & KBD_FLAG_ALT_R){
                    SelectTTY(1);
                    //DeviceIoctl(tty->conDevno, CON_CMD_SELECT_CON, 1);
				}
				break;
			case KBD_F3:
				//shft +f4 关闭窗口程序
				//TTY_PutKey(tty,  F3);
				if(key & KBD_FLAG_ALT_L || key & KBD_FLAG_ALT_R){
                    SelectTTY(2);
					//DeviceIoctl(tty->conDevno, CON_CMD_SELECT_CON, 2);
				}
				break;
			case KBD_F4:
				//alt +f4 关闭程序
				TTY_PutKey(tty,  KBD_F4);
                if(key & KBD_FLAG_ALT_L || key & KBD_FLAG_ALT_R){
                    SelectTTY(3);
					//DeviceIoctl(tty->conDevno, CON_CMD_SELECT_CON, 3);
				}
				break;	
			default:
				break;
		    }
            /* 发送按键到图形tty，按下和弹起都接收 */
            TTY_PutKey(tty, key);
        } else {
            /* 只获取按下键，不处理弹起 */
            if (!(key & KBD_FLAG_BREAK)) {
                switch(raw_code) {
                case KBD_ENTER:
                    TTY_PutKey(tty, '\n');
                    break;
                case KBD_BACKSPACE:
                    TTY_PutKey(tty,  '\b');
                    break;
                case KBD_TAB:
                    TTY_PutKey(tty,  '\t');
                    
                    break;
                case KBD_F1:
                    //TTY_PutKey(tty,  F1);
                    if(key & KBD_FLAG_ALT_L || key & KBD_FLAG_ALT_R){
                        SelectTTY(0);
                        //DeviceIoctl(tty->conDevno, CON_CMD_SELECT_CON, 0);
                    }
                    break;
                case KBD_F2:
                    //TTY_PutKey(tty,  F2);
                    if(key & KBD_FLAG_ALT_L || key & KBD_FLAG_ALT_R){
                        SelectTTY(1);
                        //DeviceIoctl(tty->conDevno, CON_CMD_SELECT_CON, 1);
                    }
                    break;
                    
                case KBD_F3:
                    //shft +f4 关闭窗口程序
                    //TTY_PutKey(tty,  F3);
                    if(key & KBD_FLAG_ALT_L || key & KBD_FLAG_ALT_R){
                        SelectTTY(2);
                        //DeviceIoctl(tty->conDevno, CON_CMD_SELECT_CON, 2);
                    }
                    break;
                case KBD_F4:
                    //alt +f4 关闭程序
                    TTY_PutKey(tty,  KBD_F4);
                    if(key & KBD_FLAG_ALT_L || key & KBD_FLAG_ALT_R){
                        SelectTTY(3);
                        //DeviceIoctl(tty->conDevno, CON_CMD_SELECT_CON, 3);
                    }
                    break;	
                case KBD_F5:
                    TTY_PutKey(tty,  KBD_F5);

                    break;	  
                case KBD_F6:
                    TTY_PutKey(tty,  KBD_F6);
                    break;	   
                case KBD_F7:
                    TTY_PutKey(tty,  KBD_F7);
                    break;	  
                case KBD_F8:
                    TTY_PutKey(tty,  KBD_F8);
                    break;	  
                case KBD_F9:
                    TTY_PutKey(tty,  KBD_F9);
                    break;	  
                case KBD_F10:
                    TTY_PutKey(tty,  KBD_F10);
                    break;	  
                case KBD_F11:
                    TTY_PutKey(tty,  KBD_F11);
                    break;	  
                case KBD_F12:  
                    
                    //TTY_PutKey(tty,  F12);
                    DeviceIoctl(tty->conDevno, CON_CMD_CLEAN, 0);
                    break;
                case KBD_ESC:
                    TTY_PutKey(tty, KBD_ESC);
                    
                    break;	
                case KBD_UP:
                    if (key & KBD_FLAG_CTRL_L || key & KBD_FLAG_CTRL_R) {
                        DeviceIoctl(tty->conDevno, CON_CMD_SCROLL, SCREEN_UP);
                    }
                    TTY_PutKey(tty, KBD_UP);
                    
                    break;
                case KBD_DOWN:
                    if (key & KBD_FLAG_CTRL_L || key & KBD_FLAG_CTRL_R) {
                        DeviceIoctl(tty->conDevno, CON_CMD_SCROLL, SCREEN_DOWN);
                    }
                    TTY_PutKey(tty, KBD_DOWN);
                    
                    break;
                case KBD_LEFT:
                    
                    TTY_PutKey(tty, KBD_LEFT);
                    break;
                case KBD_RIGHT:
                    TTY_PutKey(tty, KBD_RIGHT);
                    break;
                case KBD_PAGEUP:
                    TTY_PutKey(tty, KBD_PAGEUP);
                    break;
                case KBD_PAGEDOWN:
                    TTY_PutKey(tty, KBD_PAGEDOWN);
                    break;	
                case KBD_HOME:
                    TTY_PutKey(tty, KBD_HOME);
                    break;	
                case KBD_END:
                    TTY_PutKey(tty, KBD_END);
                    break;	
                case KBD_INSERT:
                    TTY_PutKey(tty, KBD_INSERT);
                    break;	
                case KBD_DELETE:
                    TTY_PutKey(tty, KBD_DELETE);
                    break;	
                default:
                    break;
                }
            }
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
    /* 最后一个tty不读取键盘 */
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
        IoQueueGet(&tty->ioqueue);
        /* 写入控制台 */
        //DevicePutc(tty->conDevno, IoQueueGet(&tty->ioqueue));
        //key = 0;
    }
}

/**
 * TTY_Read - tty终端读取接口
 * @tty: 哪个tty
 * @buf: 缓冲区
 * @len: 读取的数据长度（字节）
 * 
 * 从tty读取键盘输入,根据len读取不同的长度
 */
PRIVATE int TTY_Read(struct Device *device, unsigned int off, void *buffer, unsigned int len)
{
    /* 获取控制台 */
    struct CharDevice *chrdev = (struct CharDevice *)device;
    TTY_t *tty = (TTY_t *)chrdev->private;
    int retval = -1;
    /* 如果是当前控制台，才会进行键盘的读取 */
	if (IS_CURRENT_TTY(tty)) {

        /* 如果是最后一个tty，就能直接读取数据 */
        if (tty->holdPid == CurrentTask()->pid || tty->deviceID == TTY_GRAPH_ID) {
            if (len == 1) {
                *(char *)buffer = (char )tty->key;
            } else if (len == 2) {
                *(short *)buffer = (short )tty->key;
            } else if (len == 4) {
                *(int *)buffer = (int )tty->key;
            }
            if (tty->key) {
                /* 获取按键成功 */
                retval = 0;
            }
            tty->key = KEYCODE_NONE;
            
        } else {
            /* 不是前台任务进行读取，就会产生SIGTTIN */
            SysKill(CurrentTask()->pid, SIGTTIN);
        }
    }
    /* 获取按键失败 */
    return retval;
}

/**
 * TTY_Getc - tty终端读取字符
 * @device: 设备指针
 * 
 * 从tty设备读取一个字符
 */
PRIVATE int TTY_Getc(struct Device *device)
{
    /* 获取控制台 */
    struct CharDevice *chrdev = (struct CharDevice *)device;
    TTY_t *tty = (TTY_t *)chrdev->private;
    
    int key = KEYCODE_NONE;
    /* 如果是当前控制台，才会进行键盘的读取 */
	if (IS_CURRENT_TTY(tty)) {
        /* 如果是最后一个tty，就能直接读取数据 */
        if (tty->holdPid == CurrentTask()->pid || tty->deviceID == TTY_GRAPH_ID) {
            //printk("getc");
            /*if (!IoQueueEmpty(&tty->ioqueue)) {
                key = IoQueueGet(&tty->ioqueue);
            }*/
            key = tty->key;
            tty->key = KEYCODE_NONE;
            
        } else {
            /* 不是前台任务进行读取，就会产生SIGTTIN */
            SysKill(CurrentTask()->pid, SIGTTIN);
            
        }
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
    
    /* 前台任务 */
    if (tty->holdPid == CurrentTask()->pid  || CurrentTask()->pid == 0) {
        if (tty->deviceID < MAX_TTY_NR - 1) {
            char *p = (char *)buffer;
            int i = len;
            while (i) {
                /* 输出字符到控制台 */
                DevicePutc(tty->conDevno, *p++);
                i--;
            }
        }
        
    } else {
        /* 不是前台任务进行写入，就会产生SIGTTOU */
        SysKill(CurrentTask()->pid, SIGTTOU);
        return -1;
    }
    return 0;
}

/**
 * TTY_Putc - tty终端写入接口
 * @device: 设备指针
 * @ch: 字符
 * 
 * 往tty对应的控制台写入字符数据
 */
PRIVATE int TTY_Putc(struct Device *device, unsigned int ch)
{
    /* 获取控制台 */
    struct CharDevice *chrdev = (struct CharDevice *)device;
    TTY_t *tty = (TTY_t *)chrdev->private;
    
    /* 前台任务 */
    if (tty->holdPid == CurrentTask()->pid || CurrentTask()->pid == 0) {
        if (tty->deviceID < MAX_TTY_NR - 1) {   
            /* 输出字符到控制台 */
            DevicePutc(tty->conDevno, ch);
        }
    } else {
        /* 不是前台任务进行写入，就会产生SIGTTOU */
        SysKill(CurrentTask()->pid, SIGTTOU);
        return -1;
    }
    
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
    case TTY_CMD_HOLD:
        tty->holdPid = arg;
        //printk("tty set hold pid %d\n", arg);
        break;
	default:
		/* 失败 */
		retval = -1;
		break;
	}

	return retval;
}


PRIVATE struct DeviceOperations ttyOpSets = {
	.read = TTY_Read,
	.write = TTY_Write,
    .getc = TTY_Getc,
    .putc = TTY_Putc,
    .ioctl = TTY_Ioctl,
};

PRIVATE int TTY_InitOne(TTY_t *tty)
{
    int id = tty - ttyTable;
    tty->deviceID = id;
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
	printk("tty name %s\n", devname);
	/* 把字符设备添加到系统 */
	AddCharDevice(tty->chrdev);

    unsigned char *buf = kmalloc(IQ_BUF_LEN_8, GFP_KERNEL);
    if (buf == NULL)
        return -1;
    /* 初始化io队列 */
    IoQueueInit(&tty->ioqueue, buf, IQ_BUF_LEN_8, IQ_FACTOR_8);
    
    /* 生成控制台设备号 */
    tty->holdPid = 0;

    if (tty->deviceID < MAX_TTY_NR -1) {    
        tty->conDevno = MKDEV(CONSOLE_MAJOR, id);

        /* 打开控制台设备 */
        if (DeviceOpen(tty->conDevno, 0)) {
            printk("open console device failed!\n");
            return -1;
        }
    }
    return 0;
}

/**
 * TaskTTY - tty任务
 * @arg: 参数
 */
PRIVATE void TaskTTY(void *arg)
{
    /* 记得在开启tty任务之前初始化键盘 */
	TTY_t *tty;
    /* 打开键盘设备 */
    DeviceOpen(DEV_KEYBOARD, 0);

    /* 选择默认的TTY */
#ifdef CONFIG_DISPLAY_GRAPH    
    SelectTTY(TTY_GRAPH_ID);
#else
    SelectTTY(0);
#endif  /* CONFIG_DISPLAY_GRAPH */

	while (1) {
        /* 轮询全部tty，对每个tty进行读取和写入操作，只有是当前tty才会真正操作 */
		for (tty = TTY_FIRST; tty < TTY_END; tty++) {
			TTY_DoRead(tty);
			TTY_DoWrite(tty);
		}
	}
}

PUBLIC int InitTTYDriver()
{
    /* 记得在开启tty任务之前初始化键盘 */
	TTY_t *tty;
    
    /* 初始化每一个tty */
	for (tty = TTY_FIRST; tty < TTY_END; tty++) {
		TTY_InitOne(tty);
	}
    
    /* 启动tty任务 */
    ThreadStart("tty", 3, TaskTTY, "NULL");
    return 0;
}
