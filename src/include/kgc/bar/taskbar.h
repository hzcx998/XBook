/*
 * file:		include/kgc/bar/taskbar.h
 * auther:		Jason Hu
 * time:		2020/2/16
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _KGC_BAR_TASK_BAR_H
#define _KGC_BAR_TASK_BAR_H

#include <lib/types.h>
#include <lib/stdint.h>
#include <book/list.h>
#include <kgc/container/container.h>
#include <kgc/widget/button.h>
#include <kgc/window/window.h>

#define KGC_TASK_BAR_HEIGHT     48

#define KGC_TASK_BAR_SIZE     48

/* 窗口控制器 */
typedef struct KGC_WindowController {
    KGC_Button_t button;           /* 集成按钮：第一个成员 */
    struct List list;               /* 链表 */
    KGC_Window_t *window;           /* 窗口 */
    int visible;                    /* 窗口可视 */
} KGC_WindowController_t;

PUBLIC KGC_WindowController_t *KGC_CreateWindowController();
PUBLIC void KGC_WindowControllerAdd(KGC_WindowController_t *controller);
PUBLIC void KGC_WindowControllerDel(KGC_WindowController_t *controller);
PUBLIC void KGC_WindowControllerDestroy(KGC_WindowController_t *controller);

PUBLIC void KGC_WindowControllerBindWindow(KGC_WindowController_t *controller, KGC_Window_t *window);
PUBLIC void KGC_WindowControllerBindContainer(KGC_WindowController_t *controller, KGC_Container_t *container);
PUBLIC void KGC_WindowControllerActive(KGC_WindowController_t *controller);

PUBLIC void KGC_TaskBarAddWindow(KGC_Window_t *window);
PUBLIC int KGC_TaskBarDelWindow(KGC_Window_t *window);

PUBLIC KGC_WindowController_t *KGC_WindowControllerGet(KGC_Window_t *window);

PUBLIC int KGC_InitTaskBar();

#endif   /* _KGC_BAR_TASK_BAR_H */
