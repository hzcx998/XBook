/*
 * file:		include/kgc/keycode.h
 * auther:		Jason Hu
 * time:		2020/2/8
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 键值 */
#ifndef _KGC_KEYCODE_H
#define _KGC_KEYCODE_H
/* KGC-kernel graph core 内核图形核心 */

#include <lib/types.h>
#include <lib/stdint.h>

/* 图形按键修饰 */
enum KGC_KeyModify {
    KGC_KMOD_NONE = 0,        /* 无按键修饰 */

    KGC_KMOD_NUM = 0x01,      /* 数字键 */
    KGC_KMOD_CAPS = 0x02,     /* 大写键 */

    KGC_KMOD_LCTRL = 0x04,    /* 左ctrl */
    KGC_KMOD_RCTRL = 0x08,    /* 右ctrl */
    KGC_KMOD_CTRL = (KGC_KMOD_LCTRL | KGC_KMOD_RCTRL),     /* 任意ctrl */
    
    KGC_KMOD_LSHIFT = 0x20,   /* 左shift */
    KGC_KMOD_RSHIFT = 0x40,   /* 右shift */
    KGC_KMOD_SHIFT = (KGC_KMOD_LSHIFT | KGC_KMOD_RSHIFT),    /* 任意shift */
    
    KGC_KMOD_LALT = 0x100,    /* 左alt */
    KGC_KMOD_RALT = 0x200,    /* 右alt */
    KGC_KMOD_ALT = (KGC_KMOD_LALT | KGC_KMOD_RALT),     /* 任意alt */

    KGC_KMOD_PAD = 0x400,    /* 小键盘按键 */    
};

/* 内核图形按键信息 */
typedef struct KGC_KeyInfo {
    int scanCode;       /* 扫描码 */
    int code;           /* 键值 */
    int modify;         /* 修饰按键 */
} KGC_KeyInfo_t;

#endif   /* _KGC_KEYCODE_H */
