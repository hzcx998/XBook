/*
 * file:		kernel/kgc/container/draw.c
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>
#include <kgc/container/container.h>
#include <kgc/container/draw.h>
#include <lib/string.h>

PUBLIC void KGC_ContainerReadPixel(KGC_Container_t *container, int x, int y, uint32_t *color)
{
	if (x < 0 || x < 0 || x >= container->width || \
		y >= container->height) {
		return;
	}
    /* 从缓冲区读取颜色 */
	*color = *(uint32_t *)(container->buffer + (y * container->width + x));
}

PUBLIC void KGC_ContainerWritePixel(KGC_Container_t *container, int x, int y, uint32_t color)
{
	if (x < 0 || x < 0 || x >= container->width || \
		y >= container->height) {
		return;
	}
    /* 写入颜色到缓冲区 */
	*(uint32_t *)(container->buffer + (y * container->width + x)) = color;
}

void KGC_ContainerDrawRectangle(KGC_Container_t *container, int x, int y, int width, int height, uint32_t color)
{
	int i, j;
	for (j = 0; j < height; j++) {
		for (i = 0; i < width; i++) {
			KGC_ContainerWritePixel(container, x + i, y + j, color);
		}
	}
}

void KGC_ContainerDrawRectanglePlus(KGC_Container_t *container, int x, int y, int width, int height, uint32_t color)
{
    KGC_ContainerDrawRectangle(container, x, y, width, height, color);
    KGC_ContainerRefresh(container, x, y, x + width, y + height);
}

PUBLIC void KGC_ContainerDrawBitmap(KGC_Container_t *container, 
    int x, int y, int width, int height, uint32_t *bitmap)
{
	/* 对范围进行检测 */
    int x1, y1;
    uint32_t color;
    uint32_t *p;
    for (y1 = 0; y1 < height; y1++) {
        for (x1 = 0; x1 < width; x1++) {
            color = bitmap[y1 * width + x1];
            if ((color >> 24) > 0) {
                p = container->buffer + (y + y1) * container->width + (x + x1);
                *p = color;
            }
        }
    }
}

PUBLIC void KGC_ContainerDrawBitmapPlus(KGC_Container_t *container, 
    int x, int y, int width, int height, uint32_t *bitmap)
{
    KGC_ContainerDrawBitmap(container, x, y, width, height, bitmap);
    KGC_ContainerRefresh(container, x, y, x + width, y + height);
}

PUBLIC void KGC_ContainerDrawLine(KGC_Container_t *container, int x0, int y0, int x1, int y1, uint32_t color)
{
    int i, x, y, len, dx, dy;
	dx = x1 - x0;
	dy = y1 - y0;
	
	x = x0 << 10;
	y = y0 << 10;
	
	if(dx < 0){
		dx = -dx;
	}
	if(dy < 0){
		dy = -dy;
	}
	if(dx >= dy ){
		len = dx + 1;
		if(x0 > x1){
			dx = -1024;
		} else {
			dx = 1024;
			
		}
		if(y0 <= y1){
			dy = ((y1 - y0 + 1) << 10)/len;
		} else {
			dy = ((y1 - y0 - 1) << 10)/len;
		}
		
	}else{
		len = dy + 1;
		if(y0 > y1){
			dy = -1024;
		} else {
			dy = 1024;
			
		}
		if(x0 <= x1){
			dx = ((x1 - x0 + 1) << 10)/len;
		} else {
			dx = ((x1 - x0 - 1) << 10)/len;
		}	
	}
	for(i = 0; i < len; i++){
		KGC_ContainerWritePixel(container, (x >> 10), (y >> 10), color);
		x += dx;
		y += dy;
	}
}


PUBLIC void KGC_ContainerDrawLinePlus(KGC_Container_t *container, int x0, int y0, int x1, int y1, uint32_t color)
{
    KGC_ContainerDrawLine(container, x0, y0, x1, y1, color);
    KGC_ContainerRefresh(container, x0, y0, x1, y1);
}

PRIVATE void KGC_ContainerDrawCharBit(KGC_Container_t *container,
    int x, int y , uint32_t color, uint8_t *data)
{
	int i;
	char d /* data */;
	for (i = 0; i < 16; i++) {
		d = data[i];
		if ((d & 0x80) != 0)
            KGC_ContainerWritePixel(container, x + 0, y + i, color);
		if ((d & 0x40) != 0)
            KGC_ContainerWritePixel(container, x + 1, y + i, color);
		if ((d & 0x20) != 0)
             KGC_ContainerWritePixel(container, x + 2, y + i, color);
		if ((d & 0x10) != 0)
            KGC_ContainerWritePixel(container, x + 3, y + i, color);
		if ((d & 0x08) != 0)
            KGC_ContainerWritePixel(container, x + 4, y + i, color);
		if ((d & 0x04) != 0)
            KGC_ContainerWritePixel(container, x + 5, y + i, color);
		if ((d & 0x02) != 0)
            KGC_ContainerWritePixel(container, x + 6, y + i, color);
		if ((d & 0x01) != 0)
            KGC_ContainerWritePixel(container, x + 7, y + i, color);
	}	
}

PUBLIC void KGC_ContainerDrawCharWithFont(KGC_Container_t *container,
    int x, int y, char ch, uint32_t color, KGC_Font_t *font)
{
	KGC_ContainerDrawCharBit(container, x, y, color, font->addr + ch * font->height);
}

PUBLIC void KGC_ContainerDrawCharWithFontPlus(KGC_Container_t *container,
    int x, int y, char ch, uint32_t color, KGC_Font_t *font)
{
    KGC_ContainerDrawCharWithFont(container, x, y, ch, color, font);
    KGC_ContainerRefresh(container, x, y, x + font->width, y + font->height);
}

PUBLIC void KGC_ContainerDrawStringWithFont(KGC_Container_t *container,
    int x, int y, char *str, uint32_t color, KGC_Font_t *font)
{
    while (*str) {
        KGC_ContainerDrawCharWithFont(container, x, y, *str, color, font);
		x += font->width;
        str++;
	}
}

PUBLIC void KGC_ContainerDrawStringWithFontPlus(KGC_Container_t *container,
    int x, int y, char *str, uint32_t color, KGC_Font_t *font)
{
    KGC_ContainerDrawStringWithFont(container, x, y, str, color, font);
    KGC_ContainerRefresh(container, x, y, x + font->width * strlen(str), y + font->height);
}

PUBLIC void KGC_ContainerDrawChar(KGC_Container_t *container,
    int x, int y, char ch, uint32_t color)
{
    KGC_ContainerDrawCharWithFont(container, x, y, ch, color, currentFont);
}

PUBLIC void KGC_ContainerDrawCharPlus(KGC_Container_t *container,
    int x, int y, char ch, uint32_t color)
{
    KGC_ContainerDrawChar(container, x, y, ch, color);
    KGC_ContainerRefresh(container, x, y, x + currentFont->width, y + currentFont->height);
}

PUBLIC void KGC_ContainerDrawString(KGC_Container_t *container,
    int x, int y, char *str, uint32_t color)
{
    KGC_ContainerDrawStringWithFont(container, x, y, str, color, currentFont);
}

PUBLIC void KGC_ContainerDrawStringPlus(KGC_Container_t *container,
    int x, int y, char *str, uint32_t color)
{
    KGC_ContainerDrawStringWithFont(container, x, y, str, color, currentFont);
    KGC_ContainerRefresh(container, x, y, x + currentFont->width * strlen(str), y + currentFont->height);
}