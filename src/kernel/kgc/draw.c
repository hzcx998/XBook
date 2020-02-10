/*
 * file:		kernel/kgc/draw.c
 * auther:		Jason Hu
 * time:		2020/2/5
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* KGC-kernel graph core 内核图形核心 */

/* 系统内核 */
#include <book/config.h>

#ifdef CONFIG_DISPLAY_GRAPH

#include <book/arch.h>
#include <book/debug.h>
#include <book/bitops.h>
#include <lib/string.h>

/* 图形系统 */
#include <book/kgc.h>
#include <kgc/draw.h>

/**
 * KGC_DrawPixel - 绘制像素
 * @x: 横坐标
 * @y: 纵坐标
 * @color: 颜色
 */
PUBLIC void KGC_DrawPixel(int x, int y, uint32_t color)
{
    /* 绘制一个非缓冲区类型的图形 */
    KGC_CoreDraw(MERGE32(x, y), MERGE32(1, 1), NULL, color);
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
		KGC_DrawPixel((x >> 10), (y >> 10), color);
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
    /* 绘制一个非缓冲区类型的图形 */
    KGC_CoreDraw(MERGE32(x, y), MERGE32(width, height), NULL, color);
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
    /* 绘制一个缓冲区类型的图形 */
    KGC_CoreDraw(MERGE32(x, y), MERGE32(width, height), bitmap, 0);
}

/**
 * KGC_DrawCircle - 绘制圆形
 * @center_x: 中心横坐标
 * @center_y: 中心纵坐标
 * @radius: 半径
 * @color: 眼色
 */
PUBLIC void KGC_DrawCircle(uint32_t center_x,uint32_t center_y, uint32_t radius,uint32_t color)  
{
    int x, y, p;  
    x = 0, y = radius, p = 1-radius;  

    while (x < y)
    {
        KGC_DrawPixel(center_x + x, center_y + y, color);  
        KGC_DrawPixel(center_x - x, center_y + y, color);  
        KGC_DrawPixel(center_x - x, center_y - y, color);  
        KGC_DrawPixel(center_x + x, center_y - y, color);  
        KGC_DrawPixel(center_x + y, center_y + x, color);  
        KGC_DrawPixel(center_x - y, center_y + x, color);  
        KGC_DrawPixel(center_x - y, center_y - x, color);  
        KGC_DrawPixel(center_x + y, center_y - x, color);  
        x++;
        if (p < 0) p += 2*x + 1;  
        else  
        {  
            y--;  
            p += 2*x - 2*y + 1;  
        }  
    }  
}


#endif /* CONFIG_DISPLAY_GRAPH */

