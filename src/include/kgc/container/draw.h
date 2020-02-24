/*
 * file:		include/kgc/container/draw.h
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 图形容器绘图 */
#ifndef _KGC_CONTAINER_DRAW_H
#define _KGC_CONTAINER_DRAW_H

#include <lib/types.h>
#include <lib/stdint.h>
#include <kgc/font/font.h>
#include <kgc/container/container.h>

PUBLIC void KGC_ContainerReadPixel(KGC_Container_t *container,
    int x, int y, uint32_t *color);

PUBLIC void KGC_ContainerWritePixel(KGC_Container_t *container,
    int x, int y, uint32_t color);

PUBLIC void KGC_ContainerDrawRectangle(KGC_Container_t *container,
    int x, int y, int width, int height, uint32_t color);

PUBLIC void KGC_ContainerDrawBitmap(KGC_Container_t *container, 
    int x, int y, int width, int height, uint32_t *bitmap);

PUBLIC void KGC_ContainerDrawLine(KGC_Container_t *container,
    int x0, int y0, int x1, int y1, uint32_t color);

PUBLIC void KGC_ContainerDrawChar(KGC_Container_t *container,
    int x, int y, char ch, uint32_t color);

PUBLIC void KGC_ContainerDrawString(KGC_Container_t *container,
    int x, int y, char *str, uint32_t color);

PUBLIC void KGC_ContainerDrawCharWithFont(KGC_Container_t *container,
    int x, int y, char ch, uint32_t color, KGC_Font_t *font);

PUBLIC void KGC_ContainerDrawStringWithFont(KGC_Container_t *container,
    int x, int y, char *str, uint32_t color, KGC_Font_t *font);

void KGC_ContainerDrawRectanglePlus(KGC_Container_t *container,
    int x, int y, int width, int height, uint32_t color);
PUBLIC void KGC_ContainerDrawCharWithFontPlus(KGC_Container_t *container,
    int x, int y, char ch, uint32_t color, KGC_Font_t *font);
PUBLIC void KGC_ContainerDrawStringWithFontPlus(KGC_Container_t *container,
    int x, int y, char *str, uint32_t color, KGC_Font_t *font);
PUBLIC void KGC_ContainerDrawCharPlus(KGC_Container_t *container,
    int x, int y, char ch, uint32_t color);
PUBLIC void KGC_ContainerDrawStringPlus(KGC_Container_t *container,
    int x, int y, char *str, uint32_t color);
PUBLIC void KGC_ContainerDrawLinePlus(KGC_Container_t *container,
    int x0, int y0, int x1, int y1, uint32_t color);
PUBLIC void KGC_ContainerDrawBitmapPlus(KGC_Container_t *container, 
    int x, int y, int width, int height, uint32_t *bitmap);

#endif   /* _KGC_CONTAINER_DRAW_H */
