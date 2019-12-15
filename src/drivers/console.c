/*
 * file:		drivers/char/console.c
 * auther:		Jason Hu
 * time:		2019/6/26
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */


#include <book/debug.h>
#include <share/vsprintf.h>
#include <share/math.h>
#include <book/arch.h>
#include <share/string.h>
#include <book/memcache.h>
#include <drivers/console.h>
#include <drivers/tty.h>
#include <share/string.h>
#include <share/vsprintf.h>

#define DISPLAY_VRAM 0x800b8000

#define	CRTC_ADDR_REG	0x3D4	/* CRT Controller Registers - Addr Register */
#define	CRTC_DATA_REG	0x3D5	/* CRT Controller Registers - Data Register */
#define	START_ADDR_H	0xC	/* reg index of video mem start addr (MSB) */
#define	START_ADDR_L	0xD	/* reg index of video mem start addr (LSB) */
#define	CURSOR_H	0xE	/* reg index of cursor position (MSB) */
#define	CURSOR_L	0xF	/* reg index of cursor position (LSB) */
#define	V_MEM_BASE	DISPLAY_VRAM	/* base of color video memory */
#define	V_MEM_SIZE	0x8000	/* 32K: B8000H -> BFFFFH */

#define DEVNAME "console"

/* 控制台表 */
Console_t consoleTable[MAX_CONSOLE_NR];

/* 当前控制台的id，用于对多个控制台进行选择和判断 */
int currentConsoleID;

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

PRIVATE void SetCursor(unsigned short cursor)
{
	//设置光标位置 0-2000

	//执行前保存flags状态，然后关闭中断
	enum InterruptStatus oldStatus = InterruptDisable();

	Out8(CRTC_ADDR_REG, CURSOR_H);			//光标高位
	Out8(CRTC_DATA_REG, (cursor >> 8) & 0xFF);
	Out8(CRTC_ADDR_REG, CURSOR_L);			//光标低位
	Out8(CRTC_DATA_REG, cursor & 0xFF);
	//恢复之前的flags状态
	InterruptSetStatus(oldStatus);
}

PRIVATE void SetVideoStartAddr(unsigned short addr)
{
	//执行前保存flags状态，然后关闭中断
	enum InterruptStatus oldStatus = InterruptDisable();

	Out8(CRTC_ADDR_REG, START_ADDR_H);
	Out8(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	Out8(CRTC_ADDR_REG, START_ADDR_L);
	Out8(CRTC_DATA_REG, addr & 0xFF);
	//恢复之前的flags状态
	InterruptSetStatus(oldStatus);
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
		SetCursor(console->cursor);
		SetVideoStartAddr(console->currentStartAddr);
	}
}

/**
 * ConsoleClean - 清除控制台
 * @console: 控制台
 */
PRIVATE void ConsoleClean(Console_t *console)
{
    /* 设置光标和视频起始为0 */
	/*SetCursor(console->originalAddr);
	SetVideoStartAddr(console->originalAddr);
    */
    /* 指向显存 */
    unsigned char *vram = (unsigned char *)(V_MEM_BASE + console->originalAddr * 2);
	int i;
	for(i = 0; i < console->videoMemorySize; i++){
		*vram++ = 0;  /* 所有字符都置0 */
        *vram++ = COLOR_DEFAULT;  /* 颜色设置为黑白 */
	}
    console->cursor = console->originalAddr;
    console->currentStartAddr = console->originalAddr;
    Flush(console);
}

PRIVATE void DumpConsole(Console_t *console)
{
    printk(PART_TIP "----Console----\n");
    printk(PART_TIP "origin:%d size:%d current:%d cursor:%d color:%x chrdev:%x\n",
        console->originalAddr, console->currentStartAddr, console->currentStartAddr, 
        console->cursor, console->color, console->chrdev);
}

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
	if(direction == SCREEN_UP){
		if(console->currentStartAddr > console->originalAddr){
			console->currentStartAddr -= SCREEN_WIDTH;
		}
	}else if(direction == SCREEN_DOWN){
		if(console->currentStartAddr + SCREEN_SIZE < 
            console->originalAddr + console->videoMemorySize){
			console->currentStartAddr += SCREEN_WIDTH;
		}
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
        console->cursor *2) ;
	switch(ch){
		case '\n':
			if(console->cursor < console->originalAddr + 
                console->videoMemorySize - SCREEN_WIDTH){
				// 如果是回车，那还是要把回车写进去
				/**vram++ = '\n';
				*vram = 0;*/
				console->cursor = console->originalAddr + SCREEN_WIDTH *
                    ((console->cursor - console->originalAddr) / 
                    SCREEN_WIDTH + 1);
			}
			break;
		case '\b':
			if(console->cursor > console->originalAddr){
				console->cursor--;
				*(vram-2) = ' ';
				*(vram-1) = console->color;
			}
			break;
		default: 
			if(console->cursor < console->originalAddr + 
                console->videoMemorySize - 1){
				*vram++ = ch;
				*vram = console->color;
				console->cursor++;
			}
			break;
	}

	while(console->cursor >= console->currentStartAddr + SCREEN_SIZE){
		ConsoleScrollScreen(console, SCREEN_DOWN);
	}

	Flush(console);
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
    unsigned short x = (xy >> 16) & 0xffff, y = xy & 0xffff;
    /* 设置到起始位置 */
    SetVideoStartAddr(console->originalAddr);
	
    /* 设置为起始位置的某个偏移位置 */
    console->cursor = console->originalAddr + y * SCREEN_WIDTH + x;
	SetCursor(console->cursor);
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

/**
 * ConsoleInitScreen - 初始化控制台屏幕
 * @tty: 控制台所属的终端
 */
PUBLIC void ConsoleInitOne(Console_t *console, int id)
{
    /* 添加字符设备 */

    /* 初始化控制台的时候还没有内存分配，不能调用AllocCharDevice */

    /* 设置一个字符设备号 */
    CharDeviceSetDevno(&console->chrdev, MKDEV(CONSOLE_MAJOR, id));

	/* 初始化字符设备信息 */
	CharDeviceInit(&console->chrdev, 1, console);
	CharDeviceSetup(&console->chrdev, &consoleOpSets);

    char devname[DEVICE_NAME_LEN] = {0};
    sprintf(devname, "%s%d", DEVNAME, id);
	CharDeviceSetName(&console->chrdev, devname);
    
	/* 把字符设备添加到系统 */
	AddCharDevice(&console->chrdev);

	int videoMemorySize = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	/* 
    设置控制台显存大小，进行一次向下取行数，
    可以保证不跨越到其它的控制台 
    */
    int consoleVideoMemorySize = DIV_ROUND_UP(videoMemorySize / 
        MAX_CONSOLE_NR, SCREEN_WIDTH) * SCREEN_WIDTH;
    
	/* 控制台起始地址 */
    console->originalAddr      = id * consoleVideoMemorySize;
	/* 控制台占用的显存大小 */
    console->videoMemorySize   = consoleVideoMemorySize;
	/* 控制台的开始地址 */
    console->currentStartAddr  = console->originalAddr;
	/* 默认光标位置在最开始处 */
	console->cursor = console->originalAddr;
    /* 设置默认颜色 */
    console->color = COLOR_DEFAULT;

	if (id == 0) {
        ConsoleClean(console);

        /*ConsolePutChar(console, id + '0');
		ConsolePutChar(console, '#');*/
        //设定打印函数指针
        //printk = &ConsolePrint;

	} else {
        /* 输出控制台标号 */
		/*ConsolePutChar(console, id + '0');
		ConsolePutChar(console, '#');*/
	}

	SetCursor(console->cursor);
}

/**
 * ConsoleInit - 初始化控制台
 */
PUBLIC void ConsoleInit()
{
   	int i;
    /* 初始化所有控制台 */
    for (i = 0; i < MAX_CONSOLE_NR; i++) {
        ConsoleInitOne(&consoleTable[i], i);
    }
    /* 选择第一个控制台 */
    SelectConsole(0);

    /* 初始化调试输出，可以使用printk */
    InitDebugPrint();
}
