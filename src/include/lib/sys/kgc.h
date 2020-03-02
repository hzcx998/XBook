/*
 * file:		include/lib/sys/kgc.h
 * auther:		Jason Hu
 * time:		2020/2/19
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_KGC_H
#define _LIB_KGC_H

/**** 按键部分 *****/

/**** 鼠标部分 *****/

/**** 消息部分 *****/
#define KGC_MSG_SEND 1
#define KGC_MSG_RECV 2

typedef unsigned char KGC_MsgType_t; 
typedef unsigned int kgcc_t;

enum KGC_MessageTypes {
    KGC_MSG_NULL                    = 0x00, /* 空消息 */ 
    /* 输出 */
    KGC_MSG_DRAW_PIXEL              = 0x01, /* 绘制像素 */
    KGC_MSG_DRAW_RECTANGLE          = 0x02, /* 绘制矩形 */
    KGC_MSG_DRAW_BITMAP             = 0x03, /* 绘制位图 */
    KGC_MSG_DRAW_LINE               = 0x04, /* 绘制直线 */
    KGC_MSG_DRAW_CHAR               = 0x05, /* 绘制字符 */
    KGC_MSG_DRAW_STRING             = 0x06, /* 绘制字符串 */
    KGC_MSG_DRAW_PIXEL_PLUS         = 0x07, /* 绘制像素-增强 */
    KGC_MSG_DRAW_RECTANGLE_PLUS     = 0x08, /* 绘制矩形-增强 */
    KGC_MSG_DRAW_BITMAP_PLUS        = 0x09, /* 绘制位图-增强 */
    KGC_MSG_DRAW_CHAR_PLUS          = 0x0a, /* 绘制字符-增强 */
    KGC_MSG_DRAW_STRING_PLUS        = 0x0b, /* 绘制字符串-增强 */
    KGC_MSG_DRAW_LINE_PLUS          = 0x0c, /* 绘制直线-增强 */
    KGC_MSG_DRAW_UPDATE             = 0x0f, /* 绘制刷新 */
    
    /* 输入 */
    KGC_MSG_KEY_DOWN                = 0x10, /* 按键按下消息 */
    KGC_MSG_KEY_UP                  = 0x11, /* 按键弹起消息 */
    KGC_MSG_MOUSE_MOTION            = 0x20, /* 鼠标移动消息 */
    KGC_MSG_MOUSE_BUTTON_DOWN       = 0x21, /* 鼠标按钮按下消息 */
    KGC_MSG_MOUSE_BUTTON_UP         = 0x22, /* 鼠标按钮弹起消息 */
    KGC_MSG_TIMER                   = 0x23, /* 时钟产生 */
    
    /* 控制 */
    KGC_MSG_WINDOW_CREATE           = 0x30, /* 创建窗口 */
    KGC_MSG_WINDOW_CLOSE            = 0x31, /* 关闭窗口 */

    KGC_MSG_QUIT                    = 0x41, /* 退出事件监测 */

};

typedef struct KGC_MessageDraw
{
    KGC_MsgType_t type;                   /* 消息类型 */
    /* 参数 */
    int left;               /* 左位置 */
    int top;                 /* 上位置 */
    int right;              /* 右位置 */
    int buttom;             /* 下位置 */
    int width;              /* 右位置 */
    int height;             /* 下位置 */
    kgcc_t *bitmap;         /* 地图 */
    kgcc_t color;           /* 颜色 */
    char character;         /* 字符 */
    char *string;           /* 字符串 */
} KGC_MessageDraw_t;

typedef struct KGC_MessageKey
{
    KGC_MsgType_t type;                   /* 消息类型 */
    /* 参数 */
    int code;               /* 键值 */
    int modify;             /* 修饰按键 */
} KGC_MessageKey_t;


typedef struct KGC_MessageMouse
{
    KGC_MsgType_t type;                   /* 消息类型 */
    /* 参数 */
    int button;             /* 鼠标按钮 */
    int x;                  /* 鼠标位置x */
    int y;                  /* 鼠标位置y */
} KGC_MessageMouse_t;

typedef struct KGC_MessageWindow
{
    KGC_MsgType_t type;                   /* 消息类型 */
    /* 参数 */
    char *title;            /* 窗口标题 */
    int width;              /* 窗口宽度 */
    int height;             /* 窗口高度 */
} KGC_MessageWindow_t;

typedef struct KGC_MessageTimer
{
    KGC_MsgType_t type;                   /* 消息类型 */
    /* 参数 */
    unsigned long ticks;             /* 产生时的ticks */
} KGC_MessageTimer_t;

/* 消息结构 */
typedef union {
    KGC_MsgType_t type;                         /* 消息类型 */
    KGC_MessageDraw_t draw;                     /* 绘制消息 */
    KGC_MessageKey_t key;                       /* 按键 */
    KGC_MessageMouse_t mouse;                   /* 鼠标 */
    KGC_MessageWindow_t window;                 /* 窗口 */
    KGC_MessageTimer_t timer;
} KGC_Message_t;

int kgcmsg(int opereate, KGC_Message_t *msg);

#endif  /* _LIB_KGC_H */
