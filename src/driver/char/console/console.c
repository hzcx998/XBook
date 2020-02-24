/*
 * file:		char/console/console.c
 * auther:		Jason Hu
 * time:		2019/6/26
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */


#include <book/debug.h>
#include <book/arch.h>
#include <book/memcache.h>
#include <lib/vsprintf.h>
#include <lib/math.h>
#include <lib/string.h>
#include <lib/string.h>
#include <lib/vsprintf.h>

#include <char/console/console.h>
#include <char/virtual/tty.h>

#define DRV_NAME "console"

//#define DEBUG_CONSOLE

#define DISPLAY_VRAM 0x800b8000

#define	CRTC_ADDR_REG	0x3D4	/* CRT Controller Registers - Addr Register */
#define	CRTC_DATA_REG	0x3D5	/* CRT Controller Registers - Data Register */
#define	START_ADDR_H	0xC	/* reg index of video mem start addr (MSB) */
#define	START_ADDR_L	0xD	/* reg index of video mem start addr (LSB) */
#define	CURSOR_H	    0xE	/* reg index of cursor position (MSB) */
#define	CURSOR_L	    0xF	/* reg index of cursor position (LSB) */
#define	V_MEM_BASE	    DISPLAY_VRAM	/* base of color video memory */
#define	V_MEM_SIZE	    0x8000	/* 32K: B8000H -> BFFFFH */

#define COLOR_DEFAULT	(MAKE_COLOR(TEXT_BLACK, TEXT_WHITE))

/* 控制台结构 */
typedef struct Console {
    unsigned int originalAddr;          /* 控制台对应的显存的位置 */
    unsigned int screenSize;       /* 控制台占用的显存大小 */
    unsigned char color;                /* 字符的颜色 */
    int x, y;                  /* 偏移坐标位置 */
    /* 字符设备 */
    struct CharDevice *chrdev;
} Console_t;

/* 控制台表 */
Console_t consoleTable[MAX_CONSOLE_NR];

/* 当前控制台的id，用于对多个控制台进行选择和判断 */
int currentConsoleID;

#ifdef DEBUG_CONSOLE
PRIVATE unsigned short GetCursor()
{
	unsigned short posLow, posHigh;		//设置光标位置的高位的低位
	//取得光标位置
	Out8(CRTC_ADDR_REG, CURSOR_H);			//光标高位
	posHigh = In8(CRTC_DATA_REG);
	Out8(CRTC_ADDR_REG, CURSOR_L);			//光标低位
	posLow = In8(CRTC_DATA_REG);
	
	return (posHigh<<8 | posLow);	//返回合成后的值
}
#endif

PRIVATE void SetCursor(unsigned short cursor)
{
	//设置光标位置 0-2000

	//执行前保存flags状态，然后关闭中断
	unsigned long flags = InterruptSave();

	Out8(CRTC_ADDR_REG, CURSOR_H);			//光标高位
	Out8(CRTC_DATA_REG, (cursor >> 8) & 0xFF);
	Out8(CRTC_ADDR_REG, CURSOR_L);			//光标低位
	Out8(CRTC_DATA_REG, cursor & 0xFF);
	//恢复之前的flags状态
	InterruptRestore(flags);
}

PRIVATE void SetVideoStartAddr(unsigned short addr)
{
	//执行前保存flags状态，然后关闭中断
	unsigned long flags = InterruptSave();

	Out8(CRTC_ADDR_REG, START_ADDR_H);
	Out8(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	Out8(CRTC_ADDR_REG, START_ADDR_L);
	Out8(CRTC_DATA_REG, addr & 0xFF);
	//恢复之前的flags状态
	InterruptRestore(flags);
}


/**
 * IsCurrentConsole - 判断是否为当前控制台
 * @console: 控制台指针
 * 
 * 是则返回1，不是则返回0
 */
PRIVATE int IsCurrentConsole(Console_t *console)
{
    /* 如果传入的控制台是当前id对应得控制台，那么就是真 */
	return (console == &consoleTable[currentConsoleID]);
}

/**
 * Flush - 刷新光标和起始位置
 * @console: 控制台
 */
PRIVATE void Flush(Console_t *console)
{
    /* 如果是当前控制台才刷新 */
	if (IsCurrentConsole(console)) {
		/* 计算光标位置，并设置 */
        SetCursor(console->originalAddr + console->y * SCREEN_WIDTH + console->x);
		SetVideoStartAddr(console->originalAddr);
        //console->cursor = GetCursor();
	}
}

/**
 * ConsoleClean - 清除控制台
 * @console: 控制台
 */
PRIVATE void ConsoleClean(Console_t *console)
{
    /* 指向显存 */
    unsigned char *vram = (unsigned char *)(V_MEM_BASE + console->originalAddr * 2);
	int i;
	for(i = 0; i < console->screenSize * 2; i += 2){
		*vram++ = '\0';  /* 所有字符都置0 */
        *vram++ = COLOR_DEFAULT;  /* 颜色设置为黑白 */
	}
    console->x = 0;
    console->y = 0;
    Flush(console);
}

#ifdef DEBUG_CONSOLE
PRIVATE void DumpConsole(Console_t *console)
{
    printk(PART_TIP "----Console----\n");
    printk(PART_TIP "origin:%d size:%d x:%d y:%d color:%x chrdev:%x\n",
        console->originalAddr, console->screenSize, console->x, console->x, console->color, console->chrdev);
}
#endif /* DEBUG_CONSOLE */

/**
 * ConsoleScrollScreen - 滚屏
 * @console: 控制台
 * @direction: 滚动方向
 *             - SCREEN_UP: 向上滚动
 *             - SCREEN_DOWN: 向下滚动
 * 
 */
PRIVATE void ConsoleScrollScreen(Console_t *console, int direction)
{
    unsigned char *vram = (unsigned char *)(V_MEM_BASE + console->originalAddr * 2);
    int i;
                
	if(direction == SCREEN_UP){
        /* 起始地址 */
        for (i = SCREEN_WIDTH * 2 * 24; i > SCREEN_WIDTH * 2; i -= 2) {
            vram[i] = vram[i - SCREEN_WIDTH * 2];
            vram[i + 1] = vram[i + 1 - SCREEN_WIDTH * 2];
        }
        for (i = 0; i < SCREEN_WIDTH * 2; i += 2) {
            vram[i] = '\0';
            vram[i + 1] = COLOR_DEFAULT;
        }

	}else if(direction == SCREEN_DOWN){
        /* 起始地址 */
        for (i = 0; i < SCREEN_WIDTH * 2 * 24; i += 2) {
            vram[i] = vram[i + SCREEN_WIDTH * 2];
            vram[i + 1] = vram[i + 1 + SCREEN_WIDTH * 2];
        }
        for (i = SCREEN_WIDTH * 2 * 24; i < SCREEN_WIDTH * 2 * 25; i += 2) {
            vram[i] = '\0';
            vram[i + 1] = COLOR_DEFAULT;
        }
        console->y--;
	}
	Flush(console);
}

/**
 * ConsolePutChar - 控制台上输出一个字符
 * @console: 控制台
 * @ch: 字符
 */
PRIVATE void ConsolePutChar(Console_t *console, char ch)
{
	unsigned char *vram = (unsigned char *)(V_MEM_BASE + 
        (console->originalAddr + console->y * SCREEN_WIDTH + console->x) *2) ;
	switch(ch){
		case '\n':
            // 如果是回车，那还是要把回车写进去
            *vram++ = '\0';
            *vram = COLOR_DEFAULT;
            console->x = 0;
            console->y++;
            
			break;
		case '\b':
            if (console->x >= 0 && console->y >= 0) {
                console->x--;
                /* 调整为上一行尾 */
                if (console->x < 0) {
                    console->x = SCREEN_WIDTH - 1;
                    console->y--;
                    /* 对y进行修复 */
                    if (console->y < 0)
                        console->y = 0;
                }
                *(vram-2) = '\0';
				*(vram-1) = COLOR_DEFAULT;
            }
			break;
		default: 
            *vram++ = ch;
			*vram = console->color;

			console->x++;
            if (console->x > SCREEN_WIDTH - 1) {
                console->x = 0;
                console->y++;
            }
			break;
	}
    /* 滚屏 */
    while (console->y > SCREEN_HEIGHT - 1) {
        ConsoleScrollScreen(console, SCREEN_DOWN);
    }

    Flush(console);
}

/**
 * ConsoleDebugPutChar - 控制台调试输出
 * @ch:字符
 */
PUBLIC void ConsoleDebugPutChar(char ch)
{
    ConsolePutChar(&consoleTable[0], ch);    
}
/**
 * GetCurrentConsole - 获取当前控制台
 * 
 * 返回控制台的指针
 */
PUBLIC Console_t *GetCurrentConsole()
{
	return &consoleTable[currentConsoleID];
}

/**
 * SelectConsole - 选择控制台
 * @consoleID: 控制台id
 * 
 * id范围[0 ~ (MAX_CONSOLE_NR - 1)]
 */
PRIVATE void SelectConsole(int consoleID)
{
	if ((consoleID < 0) || (consoleID >= MAX_CONSOLE_NR)) {
		return;
	}

	currentConsoleID = consoleID;

    /* 刷新成当前控制台的光标位置 */
	Flush(&consoleTable[consoleID]);
}

/**
 * ConsoleWrite - 控制台写入数据
 * @device: 设备
 * @off: 偏移（未使用）
 * @buf: 缓冲区
 * @count: 字节数量
 * 
 */
PUBLIC int ConsoleWrite(struct Device *device, unsigned int off, void *buffer, unsigned int count)
{
    /* 获取控制台 */
    struct CharDevice *chrdev = (struct CharDevice *)device;
    Console_t *console = (Console_t *)chrdev->private;
    
    char *buf = (char *)buffer;

	while (count > 0 && *buf) {
		/* 输出字符到控制台 */
        ConsolePutChar(console, *buf);
		buf++;
		count--;
	}
    return 0;
}

/**
 * ConsolePutc - 往控制台设备输出一个字符
 * @device: 设备
 * @ch: 字符
 */
PRIVATE int ConsolePutc(struct Device *device, unsigned int ch)
{
    /* 获取控制台 */
    struct CharDevice *chrdev = (struct CharDevice *)device;
    Console_t *console = (Console_t *)chrdev->private;
    
    ConsolePutChar(console, ch);
    return 0;
}

/**
 * ConsoleGotoXY - 光标移动到一个指定位置
 * @xy: 位置，x是高16位，y是低16位
 */
PUBLIC void ConsoleGotoXY(Console_t *console, unsigned int xy)
{
    console->x = (xy >> 16) & 0xffff;
    console->y = xy & 0xffff;
    Flush(console);    
}

/**
 * ConsoleSetColor - 设置控制台字符颜色
 * @color: 颜色
 */
PUBLIC void ConsoleSetColor(Console_t *console, unsigned char color)
{
	console->color = color;
}

/**
 * ConsoleIoctl - 控制台的IO控制
 * @device: 设备
 * @cmd: 命令
 * @arg: 参数
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int ConsoleIoctl(struct Device *device, int cmd, int arg)
{
    /* 获取控制台 */
    struct CharDevice *chrdev = (struct CharDevice *)device;
    Console_t *console = (Console_t *)chrdev->private;
    
	int retval = 0;
	switch (cmd)
	{
    case CON_CMD_SET_COLOR:
        ConsoleSetColor(console, arg);
        break;
    case CON_CMD_SELECT_CON:
        SelectConsole(arg);
        break;
    case CON_CMD_SCROLL:
        ConsoleScrollScreen(console, arg);
        break;
    case CON_CMD_CLEAN:
        ConsoleClean(console);
        break;
    case CON_CMD_GOTO:
        ConsoleGotoXY(console, arg);
        break;
	default:
		/* 失败 */
		retval = -1;
		break;
	}

	return retval;
}


PRIVATE struct DeviceOperations consoleOpSets = {
	.ioctl = ConsoleIoctl, 
	.putc = ConsolePutc,
    .write = ConsoleWrite,
};

PUBLIC void ConsoleInitSub(Console_t *console, int id)
{

    /* 设置屏幕大小 */
    console->screenSize = SCREEN_SIZE;

	/* 控制台起始地址 */
    console->originalAddr      = id * SCREEN_SIZE;
    
    /* 设置默认颜色 */
    console->color = COLOR_DEFAULT;

    /* 默认在左上角 */
    console->x = 0;
    console->y = 0;
}

/**
 * ConsoleInitScreen - 初始化控制台屏幕
 * @tty: 控制台所属的终端
 */
PUBLIC int ConsoleInitOne(Console_t *console, int id)
{
    /* 初始化控制台信息 */
#ifdef CONFIG_CONSOLE_DEBUG
    if (id != 0) {
        ConsoleInitSub(console, id);
    }
#else 
    ConsoleInitSub(console, id);
#endif  /* CONFIG_CONSOLE_DEBUG */    
    /* 添加字符设备 */
    console->chrdev = AllocCharDevice(MKDEV(CONSOLE_MAJOR, id));
    if (console->chrdev == NULL) {
        printk("alloc console char device failed!\n");
        return -1;
    }
	/* 初始化字符设备信息 */
	CharDeviceInit(console->chrdev, 1, console);
	CharDeviceSetup(console->chrdev, &consoleOpSets);

    char devname[DEVICE_NAME_LEN] = {0};
    sprintf(devname, "%s%d", DRV_NAME, id);
	CharDeviceSetName(console->chrdev, devname);
    
	/* 把字符设备添加到系统 */
	AddCharDevice(console->chrdev);
	
    ConsoleClean(console);
	//SetCursor(console->cursor);
    return 0;
}

/**
 * InitConsoleDebugDriver - 初始化控制台调试驱动
 */
PUBLIC void InitConsoleDebugDriver()
{
    ConsoleInitSub(&consoleTable[0], 0);
    /* 选择第一个控制台 */
    SelectConsole(0);
    ConsoleClean(&consoleTable[0]);   
}
/**
 * InitConsoleDriver - 初始化控制台驱动
 */
PUBLIC int InitConsoleDriver()
{
    
   	int i;
    /* 初始化所有控制台 */
    for (i = 0; i < MAX_CONSOLE_NR; i++) {
        if (ConsoleInitOne(&consoleTable[i], i))
            return -1;
    }
    return 0;
}
