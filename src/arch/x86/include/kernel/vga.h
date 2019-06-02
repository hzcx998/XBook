/*
 * file:		arch/x86/include/kernel/vga.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _X86_VGA_H
#define _X86_VGA_H

#include <share/stdint.h>

#define VGA_VRAM 0x800b8000

/* VGA */
#define	CRTC_ADDR_REG	0x3D4	/* CRT Controller Registers - Addr Register */
#define	CRTC_DATA_REG	0x3D5	/* CRT Controller Registers - Data Register */
#define	START_ADDR_H	0xC	/* reg index of video mem start addr (MSB) */
#define	START_ADDR_L	0xD	/* reg index of video mem start addr (LSB) */
#define	CURSOR_H	0xE	/* reg index of cursor position (MSB) */
#define	CURSOR_L	0xF	/* reg index of cursor position (LSB) */
#define	V_MEM_BASE	VGA_VRAM	/* base of color video memory */
#define	V_MEM_SIZE	0x8000	/* 32K: B8000H -> BFFFFH */

#define VIDEO_WIDTH		80
#define VIDEO_HEIGHT		25

#define VIDEO_SIZE		(VIDEO_WIDTH * VIDEO_HEIGHT)
/*
color set

MAKE_COLOR(BLUE, RED)
MAKE_COLOR(BLACK, RED) | BRIGHT
MAKE_COLOR(BLACK, RED) | BRIGHT | FLASH

*/
#define TEXT_BLACK   0x0     /* 0000 */
#define TEXT_WHITE   0x7     /* 0111 */
#define TEXT_RED     0x4     /* 0100 */
#define TEXT_GREEN   0x2     /* 0010 */
#define TEXT_BLUE    0x1     /* 0001 */
#define TEXT_FLASH   0x80    /* 1000 0000 */
#define TEXT_BRIGHT  0x08    /* 0000 1000 */
#define	MAKE_COLOR(x,y)	((x<<4) | y) /* MAKE_COLOR(Background,Foreground) */

//#define COLOR_DEFAULT	(MAKE_COLOR(BLACK, WHITE))
#define COLOR_DEFAULT	(MAKE_COLOR(TEXT_BLACK, TEXT_WHITE))
//#define COLOR_GRAY		(MAKE_COLOR(BLACK, BLACK))

uint16_t vga_getCursor();
void vga_setCursor(uint16_t cursor_pos);
void vga_setVideoStartAddr(uint16_t addr);

#define SCREEN_UP -1
#define SCREEN_DOWN 1

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)

typedef struct vga_s
{
	uint32_t current_start_addr;
	uint32_t vram_addr;
	uint32_t vram_limit;
	uint32_t cursor;
	uint8_t color;
	//struct lock *lock;
}vga_t;

void init_vga();

void vga_outChar(char ch);
void vga_scrollScreen(int direction);
void vga_flush();

void vga_clean();
void vga_gotoxy(int8_t x, int8_t y);
int vga_printf(const char *fmt, ...);
int vga_outBuffer(char* buf, int len);

int (*printk)(const char *fmt, ...);

#endif

