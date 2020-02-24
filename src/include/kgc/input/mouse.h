/*
 * file:		include/kgc/input/mouse.h
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _KGC_INPUT_MOUSE_H
#define _KGC_INPUT_MOUSE_H

#include <lib/types.h>
#include <lib/stdint.h>

#include <kgc/color.h>
#include <kgc/even.h>
#include <kgc/container/container.h>

/* 鼠标填充色 */
#define KGC_MOUSE_FILL_COLOR    KGCC_BLACK
/* 鼠标边框色 */
#define KGC_MOUSE_BORDER_COLOR  KGCC_WHITE

/* 鼠标唯一色，对于唯一色，不会进行写入视图 */
#define KGC_MOUSE_UNIQUE_COLOR  KGCC_ARGB(0,0,0,0)

/* 鼠标视图大小是16*16 */
#define KGC_MOUSE_CONTAINER_SIZE     16

/* 鼠标 */
struct KGC_Mouse {
    int x, y;               /* 鼠标位置 */
    int button;             /* 按钮 */
    KGC_Container_t *container;       /* 视图 */
    uint32_t *bmp;          /* 位图数据 */   
};

EXTERN struct KGC_Mouse mouse;

PUBLIC void KGC_MouseMove(int xinc, int yinc);
PUBLIC void KGC_MouseMotion(KGC_MouseMotionEven_t *motion);
PUBLIC void KGC_MouseDown(KGC_MouseButtonEven_t *button);
PUBLIC void KGC_MouseUp(KGC_MouseButtonEven_t *button);
PUBLIC void KGC_RefreshMouse();
PUBLIC int KGC_InitMouseContainer();

#endif   /* _KGC_INPUT_MOUSE_H */
