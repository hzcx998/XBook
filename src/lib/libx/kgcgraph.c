/*
 * file:		lib/libx/graph.c
 * auther:	    Jason Hu
 * time:		2020/2/24
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <lib/graph.h>
#include <lib/string.h>

/**
 * GUI_CreateWindow - 创建一个窗口
 * @title: 标题
 * @width: 宽度
 * @height: 高度
 * 
 * 成功返回0，失败返回-1
 */
int GUI_CreateWindow(const char *title, int width, int height)
{
    KGC_Message_t msg;
    msg.type = KGC_MSG_WINDOW_CREATE;
    msg.window.title = (char *)title;
    msg.window.width = width;
    msg.window.height = height;
    return kgcmsg(KGC_MSG_SEND, &msg);
}

/**
 * GUI_CloseWindow - 关闭当前窗口
 * 
 * 成功返回0，失败返回-1
 */
int GUI_CloseWindow()
{
    KGC_Message_t msg;
    msg.type = KGC_MSG_WINDOW_CLOSE;
    return kgcmsg(KGC_MSG_SEND, &msg);
}

/**
 * GUI_DrawPixel - 绘制像素
 * @x: 横坐标
 * @y: 纵坐标
 * @color: 颜色
 * 成功返回0，失败返回-1
 */
int GUI_DrawPixel(int x, int y, unsigned int color)
{
    KGC_Message_t msg;
    msg.type = KGC_MSG_DRAW_PIXEL;
    msg.draw.left = x;
    msg.draw.top = y;
    msg.draw.color = color;
    return kgcmsg(KGC_MSG_SEND, &msg);
}

/**
 * GUI_DrawRectangle - 绘制矩形
 * @x: 横坐标
 * @y: 纵坐标
 * @width: 宽度
 * @height: 高度
 * @color: 颜色
 * 成功返回0，失败返回-1
 */
int GUI_DrawRectangle(int x, int y, int width, int height, unsigned int color)
{
    KGC_Message_t msg;
    msg.type = KGC_MSG_DRAW_RECTANGLE;
    msg.draw.left = x;
    msg.draw.top = y;
    msg.draw.width = width;
    msg.draw.height = height;
    msg.draw.color = color;
    return kgcmsg(KGC_MSG_SEND, &msg);
}

/**
 * GUI_DrawLine - 绘制直线
 * @x0: 起始横坐标
 * @y0: 起始纵坐标
 * @x1: 起始横坐标
 * @y1: 起始纵坐标
 * @color: 颜色
 * 成功返回0，失败返回-1
 */
int GUI_DrawLine(int x0, int y0, int x1, int y1, unsigned int color)
{
    KGC_Message_t msg;
    msg.type = KGC_MSG_DRAW_LINE;
    msg.draw.left = x0;
    msg.draw.top = y0;
    msg.draw.right = x1;
    msg.draw.buttom = y1;
    msg.draw.color = color;
    return kgcmsg(KGC_MSG_SEND, &msg);
}

/**
 * GUI_DrawBitmap - 绘制位图
 * @x: 横坐标
 * @y: 纵坐标
 * @width: 位图宽度
 * @height: 位图高度
 * @bitmap: 位图
 * 成功返回0，失败返回-1
 */
int GUI_DrawBitmap(int x, int y, int width, int height, unsigned int *bitmap)
{
    KGC_Message_t msg;
    msg.type = KGC_MSG_DRAW_BITMAP;
    msg.draw.left = x;
    msg.draw.top = y;
    msg.draw.width = width;
    msg.draw.height = height;
    msg.draw.bitmap = bitmap;
    return kgcmsg(KGC_MSG_SEND, &msg);
}

/**
 * GUI_DrawText - 绘制文本
 * @x: 横坐标
 * @y: 纵坐标
 * @text: 文本
 * @color: 颜色
 * 成功返回0，失败返回-1
 */
int GUI_DrawText(int x, int y, char *text, unsigned int color)
{
    KGC_Message_t msg;
    msg.type = KGC_MSG_DRAW_STRING;
    msg.draw.left = x;
    msg.draw.top = y;
    msg.draw.string = text;
    msg.draw.color = color;
    return kgcmsg(KGC_MSG_SEND, &msg);
}

/**
 * GUI_Update - 更新区域
 * @left: 左坐标
 * @top: 顶坐标
 * @right: 右坐标
 * @buttom: 底坐标
 * 成功返回0，失败返回-1
 */
int GUI_Update(int left, int top, int right, int buttom)
{
    KGC_Message_t msg;
    msg.type = KGC_MSG_DRAW_UPDATE;
    msg.draw.left = left;
    msg.draw.top = top;
    msg.draw.right = right;
    msg.draw.buttom = buttom;
    return kgcmsg(KGC_MSG_SEND, &msg);
}

/**
 * GUI_DrawPixelPlus - 绘制像素-增强
 * @x: 横坐标
 * @y: 纵坐标
 * @color: 颜色
 * 
 * 增强版本，自动更新，不用手动更新
 * 成功返回0，失败返回-1
 */
int GUI_DrawPixelPlus(int x, int y, unsigned int color)
{
    KGC_Message_t msg;
    msg.type = KGC_MSG_DRAW_PIXEL_PLUS;
    msg.draw.left = x;
    msg.draw.top = y;
    msg.draw.color = color;
    return kgcmsg(KGC_MSG_SEND, &msg);
}

/**
 * GUI_DrawRectanglePlus - 绘制矩形-增强
 * @x: 横坐标
 * @y: 纵坐标
 * @width: 宽度
 * @height: 高度
 * @color: 颜色
 * 
 * 增强版本，自动更新，不用手动更新
 * 成功返回0，失败返回-1
 */
int GUI_DrawRectanglePlus(int x, int y, int width, int height, unsigned int color)
{
    KGC_Message_t msg;
    msg.type = KGC_MSG_DRAW_RECTANGLE_PLUS;
    msg.draw.left = x;
    msg.draw.top = y;
    msg.draw.width = width;
    msg.draw.height = height;
    msg.draw.color = color;
    return kgcmsg(KGC_MSG_SEND, &msg);
}

/**
 * GUI_DrawLinePlus - 绘制直线-增强
 * @x0: 起始横坐标
 * @y0: 起始纵坐标
 * @x1: 起始横坐标
 * @y1: 起始纵坐标
 * @color: 颜色
 * 
 * 增强版本，自动更新，不用手动更新
 * 成功返回0，失败返回-1
 */
int GUI_DrawLinePlus(int x0, int y0, int x1, int y1, unsigned int color)
{
    KGC_Message_t msg;
    msg.type = KGC_MSG_DRAW_LINE_PLUS;
    msg.draw.left = x0;
    msg.draw.top = y0;
    msg.draw.right = x1;
    msg.draw.buttom = y1;
    msg.draw.color = color;
    return kgcmsg(KGC_MSG_SEND, &msg);
}

/**
 * GUI_DrawBitmapPlus - 绘制位图-增强
 * @x: 横坐标
 * @y: 纵坐标
 * @width: 位图宽度
 * @height: 位图高度
 * @bitmap: 位图
 * 
 * 增强版本，自动更新，不用手动更新
 * 成功返回0，失败返回-1
 */
int GUI_DrawBitmapPlus(int x, int y, int width, int height, unsigned int *bitmap)
{
    KGC_Message_t msg;
    msg.type = KGC_MSG_DRAW_BITMAP_PLUS;
    msg.draw.left = x;
    msg.draw.top = y;
    msg.draw.width = width;
    msg.draw.height = height;
    msg.draw.bitmap = bitmap;
    return kgcmsg(KGC_MSG_SEND, &msg);
}

/**
 * GUI_DrawTextPlus - 绘制文本
 * @x: 横坐标
 * @y: 纵坐标
 * @text: 文本
 * @color: 颜色
 * 
 * 增强版本，自动更新，不用手动更新
 * 成功返回0，失败返回-1
 */
int GUI_DrawTextPlus(int x, int y, char *text, unsigned int color)
{
    KGC_Message_t msg;
    msg.type = KGC_MSG_DRAW_STRING_PLUS;
    msg.draw.left = x;
    msg.draw.top = y;
    msg.draw.string = text;
    msg.draw.color = color;
    return kgcmsg(KGC_MSG_SEND, &msg);
}

/**
 * GUI_PollEven - 事件轮训
 * @even: 事件
 * 
 * 如果有事件，返回0，没有事件返回-1
 */
int GUI_PollEven(GUI_Even_t *even)
{
    KGC_Message_t msg;
    if (!kgcmsg(KGC_MSG_RECV, &msg)) {
        memset(even, 0, sizeof(GUI_Even_t));
        /* 获取到事件并解析之 */
        switch (msg.type) {
        case KGC_MSG_MOUSE_MOTION:
            /* 转换数据 */
            even->type = GUI_EVEN_MOUSE_MOTION;
            even->mouse.button = msg.mouse.button;
            even->mouse.x = msg.mouse.y;
            even->mouse.y = msg.mouse.y;
            break;    
        case KGC_MSG_MOUSE_BUTTON_DOWN:
            /* 转换数据 */
            even->type = GUI_EVEN_MOUSE_BUTTONDOWN;
            even->mouse.button = msg.mouse.button;
            even->mouse.x = msg.mouse.y;
            even->mouse.y = msg.mouse.y;
            break;    
        case KGC_MSG_MOUSE_BUTTON_UP:
            /* 转换数据 */
            even->type = GUI_EVEN_MOUSE_BUTTONUP;
            even->mouse.button = msg.mouse.button;
            even->mouse.x = msg.mouse.y;
            even->mouse.y = msg.mouse.y;
            break;    
        case KGC_MSG_KEY_DOWN:
            /* 转换数据 */
            even->type = GUI_EVEN_KEY_DOWN;
            even->key.code = msg.key.code;
            even->key.modify = msg.key.modify;
            break;    
        case KGC_MSG_KEY_UP:
            /* 转换数据 */
            even->type = GUI_EVEN_KEY_UP;
            even->key.code = msg.key.code;
            even->key.modify = msg.key.modify;
            break;    
        case KGC_MSG_TIMER:
            /* 转换数据 */
            even->type = GUI_EVEN_TIMER;
            even->timer.ticks = msg.timer.ticks;
            break;
        case KGC_MSG_QUIT:
            /* 转换数据 */
            even->type = GUI_EVEN_QUIT;
            break;    
        default:
            break;
        }
    } else {
        /* 没有事件 */
        return -1;
    }
    /* 获取到事件 */
    return 0;
}
