/*
 * file:		include/kgc/bar/bar.h
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _KGC_BAR_H
#define _KGC_BAR_H

#include <lib/types.h>
#include <lib/stdint.h>

#include <kgc/even.h>
#include <kgc/color.h>
#include <kgc/bar/taskbar.h>
#include <kgc/bar/menubar.h>
#include <kgc/controls/label.h>
#include <kgc/controls/button.h>

#define KGC_MENU_BAR_COLOR KGCC_ARGB(255, 50, 50, 50) 

#define KGC_TASK_BAR_COLOR KGCC_ARGB(255, 80, 80, 80) 

/* bar的结构 */
typedef struct KGC_Bar {
    KGC_Container_t *container;

    /* 控件 */
    KGC_Label_t *icon;              /* 图标控件 */
    KGC_Label_t *title;             /* 标题控件 */
    KGC_Button_t *maxim;            /* 最大化控件 */
    KGC_Button_t *minim;            /* 最小化控件 */
    KGC_Button_t *close;            /* 关闭控件 */
    KGC_Button_t *time;             /* 时间控件 */

    KGC_WindowController_t *idle;  /* 哨兵窗口控制器 */
} KGC_Bar_t;

EXTERN KGC_Bar_t kgcbar;

/* 菜单栏和任务栏的高度 */
#define KGC_BAR_HEIGHT      (KGC_MENU_BAR_HEIGHT + KGC_TASK_BAR_HEIGHT)

PUBLIC int KGC_BarPollEven();

PUBLIC int KGC_InitBarContainer();

PUBLIC void KGC_BarMouseDown(int button, int mx, int my);
PUBLIC void KGC_BarMouseUp(int button, int mx, int my);
PUBLIC void KGC_BarMouseMotion(int mx, int my);
PUBLIC int KGC_BarKeyDown(KGC_KeyboardEven_t *even);
PUBLIC int KGC_BarKeyUp(KGC_KeyboardEven_t *even);
PUBLIC void KGC_BarDoTimer(int ticks);

#endif   /* _KGC_BAR_H */
