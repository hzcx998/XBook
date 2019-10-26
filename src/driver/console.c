/*
 * file:		driver/char/console.c
 * auther:		Jason Hu
 * time:		2019/6/26
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */


#include <book/debug.h>
#include <share/vsprintf.h>
#include <book/arch.h>
#include <share/string.h>
#include <book/memcache.h>

#define DISPLAY_VRAM 0x800b8000

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
PRIVATE struct Private {
	unsigned int currentStartAddr;
	unsigned int vramAddr;
	unsigned int vramLimit;
	unsigned int cursor;
	unsigned char color;
	unsigned int cursorRead;
}private;

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

PRIVATE void SetCursor(unsigned short cursor_pos)
{
	//设置光标位置 0-2000

	//执行前保存flags状态，然后关闭中断
	enum InterruptStatus oldStatus = InterruptDisable();

	Out8(CRTC_ADDR_REG, CURSOR_H);			//光标高位
	Out8(CRTC_DATA_REG, (cursor_pos >> 8) & 0xFF);
	Out8(CRTC_ADDR_REG, CURSOR_L);			//光标低位
	Out8(CRTC_DATA_REG, cursor_pos & 0xFF);
	//恢复之前的flags状态
	InterruptSetStatus(oldStatus);
}

PRIVATE void SetVideoStartAdr(unsigned short addr)
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

PRIVATE void Flush()
{
	SetCursor(private.cursor);
	SetVideoStartAdr(private.currentStartAddr);
}
PRIVATE void ScrollScreen(int direction)
{
	if(direction == SCREEN_UP){
		if(private.currentStartAddr > private.vramAddr){
			private.currentStartAddr -= SCREEN_WIDTH;
		}
	}else if(direction == SCREEN_DOWN){
		if(private.currentStartAddr + SCREEN_SIZE < private.vramAddr + private.vramLimit){
			private.currentStartAddr += SCREEN_WIDTH;
		}
	}
	Flush();
}

PRIVATE void SetChar(unsigned char ch)
{
	unsigned char *vram = (unsigned char *)(V_MEM_BASE + private.cursor *2) ;
	switch(ch){
		case '\n':
			if(private.cursor < private.vramAddr + private.vramLimit - SCREEN_WIDTH){
				//如果是回车，那还是要把回车写进去
				*vram++ = '\n';
				*vram = 0;
				private.cursor = private.vramAddr + SCREEN_WIDTH*((private.cursor - private.vramAddr)/SCREEN_WIDTH+1);
			}
			break;
		case '\b':
			if(private.cursor > private.vramAddr){
				private.cursor--;
				*(vram-2) = ' ';
				*(vram-1) = private.color;
			}
			break;
		default: 
			if(private.cursor < private.vramAddr + private.vramLimit - 1){
				*vram++ = ch;
				*vram = private.color;
				private.cursor++;
			}
			break;
	}
	while(private.cursor >= private.currentStartAddr + SCREEN_SIZE){
		ScrollScreen(SCREEN_DOWN);
	}
	Flush();
}

PRIVATE unsigned char GetChar()
{
	//如果在屏幕范围内
	if(private.cursorRead < SCREEN_SIZE){
		unsigned char *vram = (unsigned char *)(V_MEM_BASE + private.cursorRead *2) ;
		private.cursorRead++;
		return *vram;
	}
	return 0;
}

PRIVATE void CleanScreen()
{
	private.cursor = private.vramAddr;
	private.currentStartAddr = private.vramAddr;
	Flush();
	uint8_t *vram = (uint8_t *)(V_MEM_BASE + private.cursor *2) ;
	int i;
	for(i = 0; i < private.vramLimit*2; i+=2){
		*vram = 0;
		vram += 2;
	}
}

PUBLIC void ConsoleInit()
{
   	/*
	初始化基于硬件抽象层的显示器
	*/
	private.vramAddr = 0;
	private.vramLimit = V_MEM_SIZE>>1;
	private.currentStartAddr = 0;
	
	private.cursor = 80*5;
	private.cursorRead = 0;

	private.color = COLOR_DEFAULT;
	
	SetCursor(private.cursor);

	CleanScreen();
	
   	//设定打印函数指针
   	printk = &ConsolePrint;

	filterk = &ConsoleFliter;
}

PUBLIC void ConsoleWrite(char *buf, unsigned int count)
{

	while (count > 0 && *buf) {
		SetChar(*buf);
		buf++;
		count--;
	}
}

PUBLIC void ConsoleRead(char *buf, unsigned int count)
{
	//HalRead("display",(unsigned char *)buf, count);

	int i = 0;
	while (i < count)
	{
		//从显存中获取一个字符
		buf[i] = GetChar();
		i++;
	}
}

PUBLIC int ConsolePrint(const char *fmt, ...)
{
	int i;
	char buf[256];
	va_list arg = (va_list)((char*)(&fmt) + 4); /*4是参数fmt所占堆栈中的大小*/
	i = vsprintf(buf, fmt, arg);

	ConsoleWrite((char *)buf, i);

	return i;
}

/*
 * ConsoleFliter - 控制台信息过滤
 * @buf: 过滤到哪个部分
 * @count: 要求过滤多少数据
 * 
 * 返回实际过滤到的字符数量
 * 注：它需要和ConsoleReadGotoXY一起使用，才能达到最好的效果
 */
PUBLIC int ConsoleFliter(char *buf, unsigned int count)
{
	//申请一样大的内存来暂时存放
	char *tmp = kmalloc(PAGE_SIZE, GFP_KERNEL);
	
	//整个读取过来
	ConsoleRead(tmp, count);
	//过滤出非0的字符
	int i = 0, j = 0;
	while (j < count)
	{
		if (tmp[j] != '\0') {
			buf[i] = tmp[j];
			i++;
		}
		j++;
	}
	//释放
	kfree(tmp);

	//在buf的最后添加一个0，表示字符结束
	buf[count - 1] = '\0';
	return i;
}

PUBLIC void ConsoleGotoXY(unsigned short x, unsigned short y)
{

	private.cursor = y * SCREEN_WIDTH + x;
	SetCursor(private.cursor);

}

PUBLIC void ConsoleSetColor(unsigned char color)
{
	private.color = color;
	
}

PUBLIC void ConsoleReadGotoXY(unsigned short x, unsigned short y)
{
	private.cursorRead = y * SCREEN_WIDTH + x;
	
}

PUBLIC int SysLog(char *buf)
{
	ConsoleWrite(buf, strlen(buf));
	return 0;
}
