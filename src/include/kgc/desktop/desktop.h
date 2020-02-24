/*
 * file:		include/kgc/desktop/desktop.h
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 鼠标按钮 */
#ifndef _KGC_DESKTOP_H
#define _KGC_DESKTOP_H

#include <lib/types.h>
#include <lib/stdint.h>
#include <kgc/even.h>
#include <kgc/container/container.h>

typedef struct KGC_Desktop {
    KGC_Container_t *container;         /* 容器 */

} KGC_Desktop_t;

EXTERN KGC_Desktop_t kgcDesktop;


PUBLIC int KGC_DesktopKeyUp(KGC_KeyboardEven_t *even);
PUBLIC int KGC_DesktopKeyDown(KGC_KeyboardEven_t *even);
PUBLIC void KGC_DesktopMouseMotion(int mx, int my);
PUBLIC void KGC_DesktopMouseUp(int button, int mx, int my);
PUBLIC void KGC_DesktopMouseDown(int button, int mx, int my);
PUBLIC void KGC_DesktopDoTimer(int ticks);

PUBLIC int KGC_InitDesktopContainer();

#endif   /* _KGC_DESKTOP_H */
