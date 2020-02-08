/*
 * file:		include/graph/draw.h
 * auther:		Jason Hu
 * time:		2020/2/5
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 图形绘制 */
#ifndef _KGC_DRAW_H
#define _KGC_DRAW_H
/* KGC-kernel graph core 内核图形核心 */

#include <share/types.h>
#include <share/stdint.h>

#include <graph/color.h>

PUBLIC void KGC_DrawPixel(int x, int y, uint32_t color);
PUBLIC void KGC_DrawRectangle(int x, int y, int width, int height, uint32_t color);
PUBLIC void KGC_DrawBitmap(int x, int y, int width, int height, void *bitmap);
PUBLIC void KGC_DrawLine(int x0, int y0, int x1, int y1, uint32_t color);
PUBLIC void KGC_DrawCircle(uint32_t x, uint32_t y, uint32_t radius, uint32_t color);

#endif   /* _KGC_DRAW_H */
