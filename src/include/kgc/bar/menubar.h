/*
 * file:		include/kgc/bar/menubar.h
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _KGC_BAR_MENU_BAR_H
#define _KGC_BAR_MENU_BAR_H

#include <lib/types.h>
#include <lib/stdint.h>
#include <kgc/even.h>
#include <kgc/color.h>
#include <kgc/container/container.h>

#define KGC_MENU_BAR_HEIGHT  24
struct KGC_MenuBar {
    KGC_Container_t *container;
    int count;
    kgcc_t color;
    int fps;
};
EXTERN struct KGC_MenuBar menuBar;

PUBLIC void KGC_MenuBarMouseDown(int button, int mx, int my);
PUBLIC void KGC_MenuBarMouseUp(int button, int mx, int my);
PUBLIC void KGC_MenuBarMouseMotion(int mx, int my);

PUBLIC int KGC_MenuBarKeyDown(KGC_KeyboardEven_t *even);
PUBLIC int KGC_MenuBarKeyUp(KGC_KeyboardEven_t *even);

PUBLIC void KGC_MenuBarUpdateTime();

PUBLIC int KGC_MenuBarPoll();

PUBLIC int KGC_InitMenuBar();

#endif   /* _KGC_BAR_MENU_BAR_H */
