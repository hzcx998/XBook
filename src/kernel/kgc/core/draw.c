/*
 * file:		kernel/kgc/core/draw.c
 * auther:		Jason Hu
 * time:		2020/2/5
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* KGC-kernel graph core 内核图形核心 */

/* 系统内核 */
#include <book/config.h>

#include <book/arch.h>
#include <book/debug.h>
#include <book/bitops.h>
#include <book/kgc.h>
#include <lib/string.h>
#include <video/video.h>
#include <kgc/draw.h>

/**
 * KGC_DrawPixel - 绘制像素
 * @x: 横坐标
 * @y: 纵坐标
 * @color: 颜色
 */
PUBLIC void KGC_DrawPixel(int x, int y, uint32_t color)
{
    VideoPoint_t point;
    point.x = x;
    point.y = y;
    point.color = color;
    VideoWritePixel(&point);
}

/**
 * KGC_DrawLine - 绘制像素
 * @x0: 起始横坐标
 * @y0: 起始纵坐标
 * @x1: 结束横坐标
 * @y1: 结束纵坐标
 * @color: 颜色
 */
PUBLIC void KGC_DrawLine(int x0, int y0, int x1, int y1, uint32_t color)
{
    VideoPoint_t point;
    point.color = color;

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
        point.x = (x >> 10);
        point.y = (y >> 10);
		VideoWritePixel(&point);
		x += dx;
		y += dy;
	}
}


/**
 * KGC_DrawRect - 绘制矩形
 * @x: 横坐标
 * @y: 纵坐标
 * @width: 宽度
 * @height: 高度
 * @color: 颜色
 */
PUBLIC void KGC_DrawRectangle(int x, int y, int width, int height, uint32_t color)
{
    VideoRect_t rect;
    rect.point.x = x;
    rect.point.y = y;
    rect.point.color = color;
    rect.width = width;
    rect.height = height;
    VideoFillRect(&rect);
}

/**
 * KGC_DrawBitmap - 绘制位图
 * @x: 横坐标
 * @y: 纵坐标
 * @width: 宽度
 * @height: 高度
 * @bitmap: 位图
 */
PUBLIC void KGC_DrawBitmap(int x, int y, int width, int height, void *bitmap)
{
    VideoBitmap_t vb;
    vb.x = x;
    vb.y = y;
    vb.width = width;
    vb.height = height;
    vb.bitmap = bitmap;
    VideoBitmapBlit(&vb);
}

/**
 * KGC_CleanVideo - 清空视频显存
 * @x: 横坐标
 * @y: 纵坐标
 * @width: 宽度
 * @height: 高度 
 * @color: 颜色
 * 
 * 清空显存的某个区域
 */
PUBLIC void KGC_CleanVideo(uint32_t color)
{
    VideoBlank(color);
}
