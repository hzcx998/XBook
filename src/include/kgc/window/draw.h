/*
 * file:		include/kgc/window/draw.h
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _KGC_WINDOW_DRAW_H
#define _KGC_WINDOW_DRAW_H

#include <lib/types.h>
#include <lib/stdint.h>

#include <kgc/container/container.h>

PUBLIC void KGC_WindowDrawPixel(
    KGC_Window_t *window, 
    int x, 
    int y, 
    uint32_t color);

PUBLIC void KGC_WindowDrawRectangle(
    KGC_Window_t *window, 
    int left, 
    int top, 
    int width, 
    int height, 
    uint32_t color);

PUBLIC void KGC_WindowDrawBitmap(
    KGC_Window_t *window, 
    int left,
    int top, 
    int width, 
    int height, 
    uint32_t *bitmap);

PUBLIC void KGC_WindowDrawLine(
    KGC_Window_t *window,
    int x0,
    int y0,
    int x1,
    int y1, 
    uint32_t color);

PUBLIC void KGC_WindowDrawChar(
    KGC_Window_t *window,
    int x,
    int y,
    char ch,
    uint32_t color);
PUBLIC void KGC_WindowDrawString(
    KGC_Window_t *window,
    int x,
    int y,
    char *str,
    uint32_t color);

#endif   /* _KGC_WINDOW_DRAW_H */
