/*
 * file:		hal/char/display.c
 * auther:		Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <hal/char/display.h>
#include <book/arch.h>


#define	CRTC_ADDR_REG	0x3D4	/* CRT Controller Registers - Addr Register */
#define	CRTC_DATA_REG	0x3D5	/* CRT Controller Registers - Data Register */
#define	START_ADDR_H	0xC	/* reg index of video mem start addr (MSB) */
#define	START_ADDR_L	0xD	/* reg index of video mem start addr (LSB) */
#define	CURSOR_H	0xE	/* reg index of cursor position (MSB) */
#define	CURSOR_L	0xF	/* reg index of cursor position (LSB) */
#define	V_MEM_BASE	DISPLAY_VRAM	/* base of color video memory */
#define	V_MEM_SIZE	0x8000	/* 32K: B8000H -> BFFFFH */

PRIVATE unsigned short DisplayGetCursor()
{
	unsigned short posLow, posHigh;		//设置光标位置的高位的低位
	//取得光标位置
	Out8(CRTC_ADDR_REG, CURSOR_H);			//光标高位
	posHigh = In8(CRTC_DATA_REG);
	Out8(CRTC_ADDR_REG, CURSOR_L);			//光标低位
	posLow = In8(CRTC_DATA_REG);
	
	return (posHigh<<8 | posLow);	//返回合成后的值
}

PRIVATE void DisplaySetCursor(unsigned short cursor_pos)
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

PRIVATE void DisplaySetVideoStartAdr(unsigned short addr)
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

PRIVATE void DisplayInit();
PRIVATE void DisplayOpen();
PRIVATE void DisplayRead(unsigned char *buffer, unsigned int counts);
PRIVATE void DisplayWrite(unsigned char *buffer, unsigned int counts);
PRIVATE void DisplayClose();
PRIVATE void DisplayIoctl(unsigned int type, unsigned int value);

struct HalOperate halOperate = {
	.Init = &DisplayInit,
	.Open = &DisplayOpen,
	.Read = &DisplayRead,
	.Write = &DisplayWrite,
	.Close = &DisplayClose,
	.Ioctl = &DisplayIoctl,
};

struct Hal hal = {
	.halOperate = &halOperate,
	"display hal",
	0,
};

struct DisplayHal displayHal = {
	.hal = &hal,
};

PRIVATE void DisplayFlush()
{
	DisplaySetCursor(displayHal.cursor);
	DisplaySetVideoStartAdr(displayHal.currentStartAddr);
}

PRIVATE void DisplayScrollScreen(int direction)
{
	if(direction == SCREEN_UP){
		if(displayHal.currentStartAddr > displayHal.vramAddr){
			displayHal.currentStartAddr -= SCREEN_WIDTH;
		}
	}else if(direction == SCREEN_DOWN){
		if(displayHal.currentStartAddr + SCREEN_SIZE < displayHal.vramAddr + displayHal.vramLimit){
			displayHal.currentStartAddr += SCREEN_WIDTH;
		}
	}
	DisplayFlush();
}

PRIVATE void DisplayChar(unsigned char ch)
{
	unsigned char *vram = (unsigned char *)(V_MEM_BASE + displayHal.cursor *2) ;
	switch(ch){
		case '\n':
			if(displayHal.cursor < displayHal.vramAddr + displayHal.vramLimit - SCREEN_WIDTH){
				displayHal.cursor = displayHal.vramAddr + SCREEN_WIDTH*((displayHal.cursor - displayHal.vramAddr)/SCREEN_WIDTH+1);
			}
			break;
		case '\b':
			if(displayHal.cursor > displayHal.vramAddr){
				displayHal.cursor--;
				*(vram-2) = ' ';
				*(vram-1) = displayHal.color;
			}
			break;
		default: 
			if(displayHal.cursor < displayHal.vramAddr + displayHal.vramLimit - 1){
				*vram++ = ch;
				*vram++ = displayHal.color;
				displayHal.cursor++;
			}
			break;
	}
	while(displayHal.cursor >= displayHal.currentStartAddr + SCREEN_SIZE){
		DisplayScrollScreen(SCREEN_DOWN);
	}
	DisplayFlush();
}

PRIVATE void DisplayCleanScreen()
{
	displayHal.cursor = displayHal.vramAddr;
	displayHal.currentStartAddr = displayHal.vramAddr;
	DisplayFlush();
	uint8_t *vram = (uint8_t *)(V_MEM_BASE + displayHal.cursor *2) ;
	int i;
	for(i = 0; i < displayHal.vramLimit*2; i+=2){
		*vram = 0;
		vram += 2;
	}
}

PRIVATE void DisplaySetColor(unsigned char color)
{
	displayHal.color = color;
}

PRIVATE void DisplayResetColor()
{
	displayHal.color = COLOR_DEFAULT;
}

/*
 *抽象层函数接口
 */

PRIVATE void DisplayOpen()
{
	displayHal.isOpen++;
}

PRIVATE void DisplayClose()
{
	displayHal.isOpen--;
	if (displayHal.isOpen < 0) {
		displayHal.isOpen = 0;
	}
}

PRIVATE void DisplayRead(unsigned char *buffer, unsigned int count)
{
	
}

PRIVATE void DisplayWrite(unsigned char *buffer, unsigned int count)
{
	if (displayHal.isOpen) {
		while (*buffer && count > 0) {
			DisplayChar(*buffer);
			buffer++;
			count--;
		}
	}
}

PRIVATE void DisplayIoctl(unsigned int type, unsigned int value)
{
	if (displayHal.isOpen) {

		//根据类型设置不同的值
		switch (type)
		{
		case DISPLAY_HAL_IO_COLOR:
			DisplaySetColor(value);
			break;
		case DISPLAY_HAL_IO_CURSOR:
			displayHal.cursor = value;
			DisplaySetCursor(displayHal.cursor);
			break;
		case DISPLAY_HAL_IO_CLEAR:

			DisplayCleanScreen();
			break;
		

		default:
			break;
		}
	}
}

PRIVATE void DisplayInit()
{
	displayHal.isOpen = 0;


	displayHal.vramAddr = 0;
	displayHal.vramLimit = V_MEM_SIZE>>1;
	displayHal.currentStartAddr = 0;
	
	displayHal.cursor = 80*5;
	
	displayHal.color = COLOR_DEFAULT;
	
	DisplaySetCursor(displayHal.cursor);

	DisplayChar('D');
}
