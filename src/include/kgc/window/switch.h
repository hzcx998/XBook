/*
 * file:		include/kgc/window/switch.h
 * auther:		Jason Hu
 * time:		2020/2/23
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _KGC_WINDOW_SWITCH_H
#define _KGC_WINDOW_SWITCH_H

#include <lib/types.h>
#include <lib/stdint.h>
#include <kgc/window/window.h>

PUBLIC void KGC_SwitchWindow(KGC_Window_t *window);
PUBLIC void KGC_SwitchFirstWindow();
PUBLIC void KGC_SwitchLastWindow();
PUBLIC void KGC_SwitchNextWindow(KGC_Window_t *window);
PUBLIC void KGC_SwitchNextWindowAuto();
PUBLIC void KGC_SwitchPrevWindow(KGC_Window_t *window);
PUBLIC void KGC_SwitchPrevWindowAuto();

#endif   /* _KGC_WINDOW_SWITCH_H */
