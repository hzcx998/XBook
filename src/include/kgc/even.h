/*
 * file:		include/kgc/even.h
 * auther:		Jason Hu
 * time:		2020/2/6
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 图形事件 */
#ifndef _KGC_EVEN_H
#define _KGC_EVEN_H
/* KGC-kernel graph core 内核图形核心 */

#include <lib/types.h>
#include <lib/stdint.h>

#include <kgc/keycode.h>

/* 图形事件状态 */
enum KGC_EvenStates  {
    KGC_PRESSED   = 1,    /* 按下状态 */
    KGC_RELEASED,         /* 释放状态 */
};

/* 图形键盘输入 */
typedef struct KGC_KeyboardEven {
    u8 type;                /* 键盘事件类型：KGC_KEY_DOWN, KGC_KEY_UP */
    u8 state;               /* 按钮状态 */
    KGC_KeyInfo_t keycode;    /* 按键信息 */
} KGC_KeyboardEven_t;

/* 图形鼠标移动 */
typedef struct KGC_MouseMotionEven {
    u8 type;                /* 鼠标移动事件类型：KGC_MOUSE_MOTION */
    int32_t x;              /* x偏移 */
    int32_t y;              /* y偏移 */
} KGC_MouseMotionEven_t;

/* 图形鼠标按扭 */
typedef struct KGC_MouseButtonEven {
    u8 type;                /* 鼠标按钮事件类型：KGC_MOUSE_DOWN, KGC_MOUSE_UP */
    u8 state;               /* 按钮状态 */
    u8 button;              /* 按钮值 */
} KGC_MouseButtonEven_t;

/* 图形的输入类型 */
typedef enum {
    KGC_NOEVENT = 0,      /* 未使用 */
    KGC_KEY_DOWN,         /* 按键按下 */
    KGC_KEY_UP,           /* 按键弹起 */
    KGC_MOUSE_MOTION,     /* 鼠标移动 */
    KGC_MOUSE_BUTTON_DOWN,/* 鼠标按钮按下 */
    KGC_MOUSE_BUTTON_UP,  /* 鼠标按钮弹起 */
    MAX_KGC_EVENT_NR,     /* 最大的事件数量 */
} KGC_EvenType_t;

/* 图形输入
一个联合结构
 */
typedef union KGC_Even {
    u8 type;    /* 输入类型，由于下面每一个成员的第一个成员都是type，所以次type就是该成员的type */
    KGC_KeyboardEven_t key;
    KGC_MouseMotionEven_t motion;
    KGC_MouseButtonEven_t button;    
} KGC_Even_t;

PUBLIC void KGC_EvenKeyboardKey(int key, KGC_KeyboardEven_t *even);
PUBLIC void KGC_EvenMouseMotion(int32_t x, int32_t y, int32_t z, KGC_MouseMotionEven_t *even);
PUBLIC void KGC_EvenMouseButton(uint8_t button, KGC_MouseButtonEven_t *even);

#endif   /* _KGC_EVEN_H */
