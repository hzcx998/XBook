/*
 * file:		include/kgc/window/window.h
 * auther:		Jason Hu
 * time:		2020/2/14
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _KGC_WINDOW_H
#define _KGC_WINDOW_H

#include <lib/types.h>
#include <lib/stdint.h>
#include <book/list.h>
#include <book/spinlock.h>

#include <kgc/even.h>
#include <kgc/container/container.h>
#include <kgc/widget/button.h>

/* 最大的窗口宽度与高度，不能支持更大 */
#define KGC_MAX_WINDOW_WIDTH    4096
#define KGC_MAX_WINDOW_HEIGHT    4096

#define KGC_WINDOW_TITLE_LEN   128

#define KGC_WINDOW_NAME_LEN   24

enum KGC_WindowFlags {
    /* 模式位 */
    KGC_WINDOW_FLAG_FIX            = (1 << 0), /* 固定大小 */
    KGC_WINDOW_FLAG_AUTO           = (1 << 1), /* 自动适配 */
    KGC_WINDOW_FLAG_FULL           = (1 << 2), /* 全屏 */
    
    KGC_WINDOW_FLAG_KEY_SHORTCUTS   = (1 << 3), /* 获取系统快捷键位 */
};

enum KGC_WindowStyle {
    KGC_WINSTYPLE_BORDER        = 0X01,    /* 有单线边框 */
    KGC_WINSTYPLE_CAPTION       = 0X02,    /* 有标题栏 */
    KGC_WINSTYPLE_MAXIMIZEBOX   = 0X04,    /* 有最大化按钮 */
    KGC_WINSTYPLE_MINIMIZEBOX   = 0X08,    /* 有最小化按钮 */
    KGC_WINSTYPLE_VISIBLE       = 0X10,    /* 窗口可见 */
};

#define DEFAULT_KGC_WINDOW_STYLE    (KGC_WINSTYPLE_BORDER | \
    KGC_WINSTYPLE_CAPTION | KGC_WINSTYPLE_MAXIMIZEBOX | \
    KGC_WINSTYPLE_MINIMIZEBOX | KGC_WINSTYPLE_VISIBLE)

typedef struct KGC_WindowSkin {
    uint32_t boderColor;        /* 边框颜色 */
    uint32_t captionColor;        /* 标题栏颜色 */
    uint32_t titleColor;        /* 标题颜色 */
    uint32_t blankColor;        /* 空白颜色 */
    uint32_t boderActiveColor;  /* 边框激活颜色 */
    uint32_t titleActiveColor;  /* 边框激活颜色 */
    
} KGC_WindowSkin_t;

typedef struct KGC_WindowWidget {
    /* 控件 */
    KGC_Label_t *icon;              /* 图标控件 */
    KGC_Label_t *title;             /* 标题控件 */
    KGC_Button_t *maxim;            /* 最大化控件 */
    KGC_Button_t *minim;            /* 最小化控件 */
    KGC_Button_t *close;            /* 关闭控件 */
    
} KGC_WindowWidget_t;

/* 窗口结构 */
typedef struct KGC_Window {
    struct List list;                           /* 在全局窗口中的链表 */
    int x, y;                                   /* 窗口的位置 */
    int width, height;                          /* 窗口的宽高 */
    uint32_t style;                             /* 窗口风格 */
    KGC_Container_t *container;                                 /* 窗口所在的视图 */
    uint32_t flags;                             /* 窗口标志 */
    /* 窗口对应的任务，避免互相引用，使用空指针，使用时转换 */
    void *task;
    
    struct List messageListHead;                /* 消息队列头 */
    Spinlock_t messageLock;                     /* 消息锁 */
    char name[KGC_WINDOW_NAME_LEN];      /* 窗口名字 */
    char title[KGC_WINDOW_TITLE_LEN];      /* 窗口的标题 */
    struct KGC_Window *parentWindow;        /* 父窗口 */
    struct List childList;                  /* 子窗口链表 */
    KGC_WindowWidget_t widgets;             /* 控件 */
} KGC_Window_t;

/* 移动窗口时缩减显示,1表示开启，0表示关闭 */
#define KGC_WINDOW_MOVEMENT_SHRINK  1

/* 窗口移动的数据结构 */
typedef struct KGC_WindowMovement {
    KGC_Window_t *holded;             /* 抓取的窗口 */
#if KGC_WINDOW_MOVEMENT_SHRINK == 1
    KGC_Container_t *walker;          /* 移动的容器 */
#endif /* KGC_WINDOW_MOVEMENT_SHRINK */
    int offsetX, offsetY;           /* 抓取时鼠标和窗口的偏移 */
} KGC_WindowMovement_t;




EXTERN KGC_Window_t *currentWindow;

#define SET_CURRENT_WINDOW(window)          \
    do {                                    \
        if (window) currentWindow = window; \
    } while (0)

#define GET_CURRENT_WINDOW()  currentWindow

/* 设置窗口的标志 */
#define KGC_WINDOW_SET_FLAG(win, flag) \
    (win)->flags |= (flag)

/* 取消窗口的标志 */
#define KGC_WINDOW_UNSET_FLAG(win, flag) \
    (win)->flags &= ~(flag)

/* 取消窗口的标志 */
#define KGC_WINDOW_BIND_TASK(win, _task) \
    (win)->task = _task

/* 取消窗口的标志 */
#define KGC_WINDOW_CLEAR_TASK(win) \
    (win)->task = NULL

PUBLIC int KGC_InitWindowContainer();

PUBLIC KGC_Window_t *KGC_WindowCreate(
    char *name,
    char *title,
    uint32_t style,
    int x,
    int y,
    int width,
    int height,
    void *param);

PUBLIC int KGC_WindowAdd(KGC_Window_t *window);
PUBLIC int KGC_WindowDestroy(KGC_Window_t *window);
PUBLIC void KGC_WindowErase(KGC_Window_t *window);
PUBLIC int KGC_WindowDel(KGC_Window_t *window);
PUBLIC int KGC_WindowClose(KGC_Window_t *window);

PUBLIC void KGC_SwitchWindow(KGC_Window_t *window);
PUBLIC void KGC_SwitchFirstWindow();
PUBLIC void KGC_SwitchLastWindow();
PUBLIC void KGC_SwitchNextWindow(KGC_Window_t *window);
PUBLIC void KGC_SwitchNextWindowAuto();
PUBLIC void KGC_SwitchPrevWindow(KGC_Window_t *window);
PUBLIC void KGC_SwitchPrevWindowAuto();

PUBLIC void KGC_WindowRefresh(
    KGC_Window_t *window,
    int left,
    int up,
    int right,
    int bottom);
PUBLIC void KGC_WindowPaint(KGC_Window_t *window);

#endif   /* _KGC_WINDOW_H */
