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

/* 最大的窗口宽度与高度，不能支持更大 */
#define KGC_MAX_WINDOW_WIDTH    4096
#define KGC_MAX_WINDOW_HEIGHT    4096

#define KGC_WINDOW_TITLE_NAME_LEN   128

enum KGC_WindowFlags {
    /* 模式位 */
    KGC_WINDOW_FLAG_FIX            = (1 << 0), /* 固定大小 */
    KGC_WINDOW_FLAG_AUTO           = (1 << 1), /* 自动适配 */
    KGC_WINDOW_FLAG_FULL           = (1 << 2), /* 全屏 */
    
    KGC_WINDOW_FLAG_KEY_SHORTCUTS   = (1 << 3), /* 获取系统快捷键位 */
    
};

/* 窗口结构 */
typedef struct KGC_Window {
    struct List list;                           /* 在全局窗口中的链表 */
    int x, y;                                   /* 窗口的位置 */
    int width, height;                          /* 窗口的宽高 */
    KGC_Container_t *container;                                 /* 窗口所在的视图 */
    uint32_t flags;                             /* 窗口标志 */
    /* 窗口对应的任务，避免互相引用，使用空指针，使用时转换 */
    void *task;
    
    struct List messageListHead;                /* 消息队列头 */
    Spinlock_t messageLock;                     /* 消息锁 */
    char title[KGC_WINDOW_TITLE_NAME_LEN];      /* 窗口的标题 */
} KGC_Window_t;

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
    char *title,
    int width,
    int height);

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

#endif   /* _KGC_WINDOW_H */
