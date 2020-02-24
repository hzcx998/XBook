/*
 * file:		include/kgc/window/even.h
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _KGC_WINDOW_EVEN_H
#define _KGC_WINDOW_EVEN_H

#include <lib/types.h>
#include <lib/stdint.h>

#include <kgc/even.h>

PUBLIC void KGC_WindowMouseDown(int button, int mx, int my);
PUBLIC void KGC_WindowMouseUp(int button, int mx, int my);
PUBLIC void KGC_WindowMouseMotion(int mx, int my);

PUBLIC int KGC_WindowKeyDown(KGC_KeyboardEven_t *even);
PUBLIC int KGC_WindowKeyUp(KGC_KeyboardEven_t *even);

PUBLIC void KGC_WindowDoTimer(int ticks);

#endif   /* _KGC_WINDOW_EVEN_H */
