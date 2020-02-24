/*
 * file:		kernel/kgc/window/draw.c
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */
/* 系统内核 */
#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>
#include <kgc/video.h>
#include <kgc/container/draw.h>
#include <kgc/window/window.h>
#include <kgc/window/draw.h>

/* 窗口绘制 */

/**
 * KGC_WindowDrawPixel - 往窗口中绘制像素
 * @window: 窗口
 * @x: 窗口中的横坐标
 * @y: 窗口中的纵坐标
 * @color: 颜色
 * 
 */
PUBLIC void KGC_WindowDrawPixel(
    KGC_Window_t *window, 
    int x, 
    int y, 
    uint32_t color)
{
    /* 判断是否在窗口内 */
    if (x < 0 || x >= window->width || y < 0 || y >= window->height)
        return;
    /* 直接绘制到视图中 */
    KGC_ContainerWritePixel(window->container, window->x + x, window->y + y, color);
}

/**
 * KGC_WindowDrawRectangle - 往窗口中绘制矩形
 * @window: 窗口
 * @left: 窗口中的横坐标
 * @top: 窗口中的纵坐标
 * @width: 矩形的宽度
 * @height: 矩形的高度
 * @color: 颜色
 * 
 */
PUBLIC void KGC_WindowDrawRectangle(
    KGC_Window_t *window, 
    int left, 
    int top, 
    int width, 
    int height, 
    uint32_t color)
{
    /* 进行窗口剪切 */
    KGC_ContainerDrawRectangle(window->container, window->x + left, window->y + top,
        width, height, color);
}

/**
 * KGC_WindowDrawBitmap - 往窗口中绘制位图
 * @window: 窗口
 * @left: 窗口中的横坐标
 * @top: 窗口中的纵坐标
 * @width: 位图的宽度
 * @height: 位图的高度
 * @bitmap: 位图地址
 * 
 */
PUBLIC void KGC_WindowDrawBitmap(
    KGC_Window_t *window, 
    int left,
    int top, 
    int width, 
    int height, 
    uint32_t *bitmap)
{
    /* 进行窗口剪切 */
    KGC_ContainerDrawBitmap(window->container, window->x + left, window->y + top,
        width, height, bitmap);
}

/**
 * KGC_WindowDrawLine - 绘制直线
 * @window: 窗口
 * @x0: 起始横坐标
 * @y0: 起始纵坐标
 * @x1: 结束横坐标
 * @y1: 结束纵坐标
 * @color: 颜色
 */
PUBLIC void KGC_WindowDrawLine(
    KGC_Window_t *window,
    int x0,
    int y0,
    int x1,
    int y1, 
    uint32_t color)
{
    KGC_ContainerDrawLine(window->container, window->x + x0, window->y + y0,
        window->x + x1, window->y + y1, color);
}

/**
 * KGC_WindowDrawChar - 绘制字符
 * @window: 窗口
 * @x: 起始横坐标
 * @y: 起始纵坐标
 * @ch: 字符
 * @color: 颜色
 */
PUBLIC void KGC_WindowDrawChar(
    KGC_Window_t *window,
    int x,
    int y,
    char ch,
    uint32_t color)
{
    KGC_ContainerDrawChar(window->container, window->x + x, window->y + y, ch, color);
}

/**
 * KGC_WindowDrawString - 绘制字符串
 * @window: 窗口
 * @x: 起始横坐标
 * @y: 起始纵坐标
 * @str: 字符串
 * @color: 颜色
 */
PUBLIC void KGC_WindowDrawString(
    KGC_Window_t *window,
    int x,
    int y,
    char *str,
    uint32_t color)
{
    KGC_ContainerDrawString(window->container, window->x + x, window->y + y, str, color);
}

/**
 * KGC_WindowRefresh - 绘制直线
 * @window: 窗口
 * @x0: 起始横坐标
 * @y0: 起始纵坐标
 * @x1: 结束横坐标
 * @y1: 结束纵坐标
 * @color: 颜色
 * 
 * 刷新时，需要刷新上面的鼠标
 */
PUBLIC void KGC_WindowRefresh(
    KGC_Window_t *window,
    int left,
    int top,
    int right,
    int bottom)
{
    /* 如果不是当前窗口，就不刷新到屏幕上 */
    /*if (window != GET_CURRENT_WINDOW())
        return;*/
    
    KGC_ContainerRefresh(window->container, window->x + left, window->y + top,
        window->x + right, window->y + bottom);
}
