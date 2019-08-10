/*
 * file:		hal/char/display.c
 * auther:		Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <hal/char/display.h>
#include <book/arch.h>

/*
 * 对显示的抽象，做了什么？
 * ->在初始化中，对显示的环境进行初始化
 * ->提供了数据的写入和读出操作
 * ->有对硬件的一些基础控制
 * 通过这些基础的东西，可以满足显示驱动的需要。
 * 感觉这个抽象有点过了，以至于和驱动几乎一样。
 * 但这个是要在以后慢慢总结才行的。
 */

#define DISPLAY_VRAM 0xc00b8000

#define	CRTC_ADDR_REG	0x3D4	/* CRT Controller Registers - Addr Register */
#define	CRTC_DATA_REG	0x3D5	/* CRT Controller Registers - Data Register */
#define	START_ADDR_H	0xC	/* reg index of video mem start addr (MSB) */
#define	START_ADDR_L	0xD	/* reg index of video mem start addr (LSB) */
#define	CURSOR_H	0xE	/* reg index of cursor position (MSB) */
#define	CURSOR_L	0xF	/* reg index of cursor position (LSB) */
#define	V_MEM_BASE	DISPLAY_VRAM	/* base of color video memory */
#define	V_MEM_SIZE	0x8000	/* 32K: B8000H -> BFFFFH */

/*
 * 私有数据结构
 */
PRIVATE struct DisplayPrivate {
	unsigned int currentStartAddr;
	unsigned int vramAddr;
	unsigned int vramLimit;
	unsigned int cursor;
	unsigned char color;
	unsigned int cursorRead;
}displayPrivate;

PRIVATE unsigned short DisplayHalGetCursor()
{
	unsigned short posLow, posHigh;		//设置光标位置的高位的低位
	//取得光标位置
	Out8(CRTC_ADDR_REG, CURSOR_H);			//光标高位
	posHigh = In8(CRTC_DATA_REG);
	Out8(CRTC_ADDR_REG, CURSOR_L);			//光标低位
	posLow = In8(CRTC_DATA_REG);
	
	return (posHigh<<8 | posLow);	//返回合成后的值
}

PRIVATE void DisplayHalSetCursor(unsigned short cursor_pos)
{
	//设置光标位置 0-2000

	//执行前保存flags状态，然后关闭中断
	uint32_t flags = LoadEflags();
	DisableInterrupt();

	Out8(CRTC_ADDR_REG, CURSOR_H);			//光标高位
	Out8(CRTC_DATA_REG, (cursor_pos >> 8) & 0xFF);
	Out8(CRTC_ADDR_REG, CURSOR_L);			//光标低位
	Out8(CRTC_DATA_REG, cursor_pos & 0xFF);
	//恢复之前的flags状态
	StoreEflags(flags);
}

PRIVATE void DisplayHalSetVideoStartAdr(unsigned short addr)
{
	//执行前保存flags状态，然后关闭中断
	flags_t flags = LoadEflags();
	DisableInterrupt();

	Out8(CRTC_ADDR_REG, START_ADDR_H);
	Out8(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	Out8(CRTC_ADDR_REG, START_ADDR_L);
	Out8(CRTC_DATA_REG, addr & 0xFF);
	//恢复之前的flags状态
	StoreEflags(flags);
}

/*
 * 定义需要用到的函数 
 */

PRIVATE void DisplayHalFlush()
{
	DisplayHalSetCursor(displayPrivate.cursor);
	DisplayHalSetVideoStartAdr(displayPrivate.currentStartAddr);
}

PRIVATE void DisplayHalScrollScreen(int direction)
{
	if(direction == SCREEN_UP){
		if(displayPrivate.currentStartAddr > displayPrivate.vramAddr){
			displayPrivate.currentStartAddr -= SCREEN_WIDTH;
		}
	}else if(direction == SCREEN_DOWN){
		if(displayPrivate.currentStartAddr + SCREEN_SIZE < displayPrivate.vramAddr + displayPrivate.vramLimit){
			displayPrivate.currentStartAddr += SCREEN_WIDTH;
		}
	}
	DisplayHalFlush();
}

PRIVATE void DisplayHalChar(unsigned char ch)
{
	unsigned char *vram = (unsigned char *)(V_MEM_BASE + displayPrivate.cursor *2) ;
	switch(ch){
		case '\n':
			if(displayPrivate.cursor < displayPrivate.vramAddr + displayPrivate.vramLimit - SCREEN_WIDTH){
				//如果是回车，那还是要把回车写进去
				*vram++ = '\n';
				*vram = 0;
				displayPrivate.cursor = displayPrivate.vramAddr + SCREEN_WIDTH*((displayPrivate.cursor - displayPrivate.vramAddr)/SCREEN_WIDTH+1);
			}
			break;
		case '\b':
			if(displayPrivate.cursor > displayPrivate.vramAddr){
				displayPrivate.cursor--;
				*(vram-2) = ' ';
				*(vram-1) = displayPrivate.color;
			}
			break;
		default: 
			if(displayPrivate.cursor < displayPrivate.vramAddr + displayPrivate.vramLimit - 1){
				*vram++ = ch;
				*vram = displayPrivate.color;
				displayPrivate.cursor++;
			}
			break;
	}
	while(displayPrivate.cursor >= displayPrivate.currentStartAddr + SCREEN_SIZE){
		DisplayHalScrollScreen(SCREEN_DOWN);
	}
	DisplayHalFlush();
}

PRIVATE unsigned char DisplayHalGetChar()
{
	//如果在屏幕范围内
	if(displayPrivate.cursorRead < SCREEN_SIZE){
		unsigned char *vram = (unsigned char *)(V_MEM_BASE + displayPrivate.cursorRead *2) ;
		displayPrivate.cursorRead++;
		return *vram;
	}
	return 0;
}

PRIVATE void DisplayHalCleanScreen()
{
	displayPrivate.cursor = displayPrivate.vramAddr;
	displayPrivate.currentStartAddr = displayPrivate.vramAddr;
	DisplayHalFlush();
	uint8_t *vram = (uint8_t *)(V_MEM_BASE + displayPrivate.cursor *2) ;
	int i;
	for(i = 0; i < displayPrivate.vramLimit*2; i+=2){
		*vram = 0;
		vram += 2;
	}
}

PRIVATE int DisplayHalRead(unsigned char *buffer, unsigned int count)
{
	int i = 0;
	while (i < count)
	{
		//从显存中获取一个字符
		buffer[i] = DisplayHalGetChar();
		i++;
	}
	return 0;
}

PRIVATE int DisplayHalWrite(unsigned char *buffer, unsigned int count)
{
	while (count > 0) {
		DisplayHalChar(*buffer);
		buffer++;
		count--;
	}
	return 0;
}

PRIVATE void DisplayHalIoctl(unsigned int cmd, unsigned int param)
{
	//根据类型设置不同的值
	switch (cmd)
	{
	case DISPLAY_HAL_IO_SET_COLOR:
		displayPrivate.color = param;
		break;
	case DISPLAY_HAL_IO_SET_CURSOR:

		displayPrivate.cursor = param;
		
		DisplayHalSetCursor(displayPrivate.cursor);
		break;
	case DISPLAY_HAL_IO_CLEAR:

		DisplayHalCleanScreen();
		break;
	case DISPLAY_HAL_IO_RDCURSOR:

		displayPrivate.cursorRead = param;
		break;

	case DISPLAY_HAL_IO_GET_CURSOR:
		//获取并放入参数地址中
		*((unsigned int *)param) = DisplayHalGetCursor();
		break;
		
	case DISPLAY_HAL_IO_RESET_COLOR:

		displayPrivate.color = COLOR_DEFAULT;

		break;
	default:
		break;
	}
}

PRIVATE void DisplayHalInit()
{
	displayPrivate.vramAddr = 0;
	displayPrivate.vramLimit = V_MEM_SIZE>>1;
	displayPrivate.currentStartAddr = 0;
	
	displayPrivate.cursor = 80*5;
	displayPrivate.cursorRead = 0;

	displayPrivate.color = COLOR_DEFAULT;
	
	DisplayHalSetCursor(displayPrivate.cursor);
}

/* hal操作函数 */
PRIVATE struct HalOperate halOperate = {
	.Init = &DisplayHalInit,
	.Read = &DisplayHalRead,
	.Write = &DisplayHalWrite,
	.Ioctl = &DisplayHalIoctl,
};

/* hal对象，需要把它导入到hal系统中 */
PUBLIC HAL(halOfDisplay, halOperate, "display");
