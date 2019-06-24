/*
 * file:		arch/x86/include/kernel/vga.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _X86_VGA_H
#define _X86_VGA_H

#include <share/stdint.h>

struct vga
{
	uint32_t current_start_addr;
	uint32_t vram_addr;
	uint32_t vram_limit;
	uint32_t cursor;
	uint8_t color;
	//struct lock *lock;
};

void init_vga();

void vga_outChar(char ch);
void vga_scrollScreen(int direction);
void vga_flush();

void vga_clean();
void vga_gotoxy(int8_t x, int8_t y);
int vga_printf(const char *fmt, ...);
int vga_outBuffer(char* buf, int len);

#endif

