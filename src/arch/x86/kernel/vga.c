/*
 * file:		arch/x86/kernel/vga.c
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by BookOS developers. All rights reserved.
 */

#include <vga.h>
#include <x86.h>
#include <share/vsprintf.h>

struct vga_s vga;

/*
 * 功能: 初始化vga视频，文本模式
 * 参数: 无
 * 返回: 无
 * 说明: 初始化一些基础信息，后面显示文本的时候需要用到这些信息
 */

void init_vga()
{
	vga.vram_addr = 0;
	vga.vram_limit = V_MEM_SIZE>>1;
	vga.current_start_addr = 0;
	
	vga.cursor = 80*5;
	
	vga.color = COLOR_DEFAULT;
	
	vga_setCursor(vga.cursor);
	
	//printk指针指向vga_printf，使用printk就相当于使用vga_printf
	printk = vga_printf;

	printk("< init vga done.\n");

}

uint16_t get_cursor()
{
	uint16_t pos_low, pos_high;		//设置光标位置的高位的低位
	//取得光标位置
	Out8(CRTC_ADDR_REG, CURSOR_H);			//光标高位
	pos_high = In8(CRTC_DATA_REG);
	Out8(CRTC_ADDR_REG, CURSOR_L);			//光标低位
	pos_low = In8(CRTC_DATA_REG);
	
	return (pos_high<<8 | pos_low);	//返回合成后的值
}

void vga_setCursor(uint16_t cursor_pos)
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

void set_video_start_addr(uint16_t addr)
{
	//执行前保存flags状态，然后关闭中断
	uint32_t flags = LoadEflags();
	DisableInterrupt();

	Out8(CRTC_ADDR_REG, START_ADDR_H);
	Out8(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	Out8(CRTC_ADDR_REG, START_ADDR_L);
	Out8(CRTC_DATA_REG, addr & 0xFF);
	//恢复之前的flags状态
	StoreEflags(flags);
}

void vga_outChar(char ch)
{
	uint8_t *vram = (uint8_t *)(V_MEM_BASE + vga.cursor *2) ;
	switch(ch){
		case '\n':
			if(vga.cursor < vga.vram_addr + vga.vram_limit - SCREEN_WIDTH){
				vga.cursor = vga.vram_addr + SCREEN_WIDTH*((vga.cursor - vga.vram_addr)/SCREEN_WIDTH+1);
			}
			break;
		case '\b':
			if(vga.cursor > vga.vram_addr){
				vga.cursor--;
				*(vram-2) = ' ';
				*(vram-1) = vga.color;
			}
			break;
		default: 
			if(vga.cursor < vga.vram_addr + vga.vram_limit - 1){
				*vram++ = ch;
				*vram++ = vga.color;
				vga.cursor++;
			}
			break;
	}
	while(vga.cursor >= vga.current_start_addr + SCREEN_SIZE){
		vga_scrollScreen(SCREEN_DOWN);
	}
	vga_flush();
}

void vga_flush()
{
	vga_setCursor(vga.cursor);
	set_video_start_addr(vga.current_start_addr);
}

void vga_scrollScreen(int direction)
{
	if(direction == SCREEN_UP){
		if(vga.current_start_addr > vga.vram_addr){
			vga.current_start_addr -= SCREEN_WIDTH;
		}
	}else if(direction == SCREEN_DOWN){
		if(vga.current_start_addr + SCREEN_SIZE < vga.vram_addr + vga.vram_limit){
			vga.current_start_addr += SCREEN_WIDTH;
		}
	}
	vga_flush();
}

void vga_clean()
{
	vga.cursor = vga.vram_addr;
	vga.current_start_addr = vga.vram_addr;
	vga_flush();
	uint8_t *vram = (uint8_t *)(V_MEM_BASE + vga.cursor *2) ;
	int i;
	for(i = 0; i < vga.vram_limit*2; i+=2){
		*vram = 0;
		vram += 2;
	}
}

void vga_gotoxy(int8_t x, int8_t y)
{
	if(x < 0){
		x = 0;
	}
	if(x > SCREEN_WIDTH - 1){
		x = SCREEN_WIDTH - 1;
	}
	
	if(y < 0){
		y = 0;
	}
	if(y > SCREEN_HEIGHT - 1){
		y = SCREEN_HEIGHT - 1;
	}
	vga.cursor = vga.vram_addr + y*SCREEN_WIDTH+x;
	vga_flush();
}

void vga_setColor(uint8_t color)
{
	vga.color = color;
}

void vga_resetColor()
{
	vga.color = COLOR_DEFAULT;
}

int vga_printf(const char *fmt, ...)
{
	int i;
	char buf[256];
	va_list arg = (va_list)((char*)(&fmt) + 4); /*4是参数fmt所占堆栈中的大小*/
	i = vsprintf(buf, fmt, arg);
	vga_outBuffer(buf, i);
	
	return i;
}

int vga_outBuffer(char* buf, int len)
{
	char* p = buf;
	int i = len;
	while (i) {
		vga_outChar(*p++);
		i--;
	}
	return len;
}
