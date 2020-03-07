/*
 * file:		include/lib/graph.h
 * auther:		Jason Hu
 * time:		2020/2/24
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_GRAPH_H
#define _LIB_GRAPH_H

#include "sys/kgc.h"

/* 生成argb颜色 */
#define GUI_ARGB(a, r, g, b) ((((a) & 0xff) << 24) | \
    (((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff)) 
#define GUI_RGB(a, r, g, b) GUI_ARGB(255, r, g, b)

/* 图形事件 */
typedef enum {
    GUI_EVEN_NONE = 0,          /* 无事件 */
    GUI_EVEN_KEY_DOWN,          /* 按键按下事件 */
    GUI_EVEN_KEY_UP,            /* 按键弹起事件 */
    GUI_EVEN_MOUSE_MOTION,      /* 鼠标移动事件 */
    GUI_EVEN_MOUSE_BUTTONDOWN,  /* 鼠标按钮按下事件 */
    GUI_EVEN_MOUSE_BUTTONUP,    /* 鼠标按钮弹起事件 */
    GUI_EVEN_TIMER,             /* 定时器产生 */
    GUI_EVEN_QUIT,              /* 退出事件 */
} GUI_EvenType_t;

/* 按键事件 */
typedef struct {
    GUI_EvenType_t type;          /* 事件类型 */ 
    int code;           /* 按键键值 */
    int modify;         /* 修饰按键 */
} GUI_EvenKey_t ;

/* 鼠标事件 */
typedef struct {
    GUI_EvenType_t type;          /* 事件类型 */ 
    int button;         /* 鼠标按钮 */
    int x;              /* 鼠标x */
    int y;              /* 鼠标y */
} GUI_EvenMouse_t ;

/* 定时器事件 */
typedef struct {
    GUI_EvenType_t type;          /* 事件类型 */ 
    int ticks;          /* 定时器产生时的ticks */
} GUI_EvenTimer_t ;

typedef union {
    GUI_EvenType_t type;    /* 事件类型 */
    GUI_EvenKey_t key;      /* 按键 */
    GUI_EvenMouse_t mouse;  /* 鼠标 */
    GUI_EvenTimer_t timer;    /* 定时器 */
} GUI_Even_t;

int GUI_CreateWindow(char *name, char *title, unsigned int style,
    int x, int y, int width, int height, void *param);
int GUI_CloseWindow();
int GUI_DrawPixel(int x, int y, unsigned int color);
int GUI_DrawRectangle(int x, int y, int width, int height, unsigned int color);
int GUI_DrawLine(int x0, int y0, int x1, int y1, unsigned int color);
int GUI_DrawBitmap(int x, int y, int width, int height, unsigned int *bitmap);
int GUI_DrawText(int x, int y, char *text, unsigned int color);
int GUI_Update(int left, int top, int right, int buttom);
int GUI_DrawPixelPlus(int x, int y, unsigned int color);
int GUI_DrawRectanglePlus(int x, int y, int width, int height, unsigned int color);
int GUI_DrawLinePlus(int x0, int y0, int x1, int y1, unsigned int color);
int GUI_DrawBitmapPlus(int x, int y, int width, int height, unsigned int *bitmap);
int GUI_DrawTextPlus(int x, int y, char *text, unsigned int color);

int GUI_PollEven(GUI_Even_t *even);

#endif /* _LIB_GRAPH_H */