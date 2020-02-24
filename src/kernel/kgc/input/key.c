/*
 * file:		kernel/kgc/input/key.c
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */
/* 系统内核 */
#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>
#include <kgc/window/window.h>
#include <kgc/window/even.h>
#include <kgc/input/key.h>
#include <kgc/input/mouse.h>
#include <kgc/bar/bar.h>
#include <kgc/desktop/desktop.h>

PUBLIC void KGC_KeyDown(KGC_KeyboardEven_t *even)
{
    /* 如果已经执行了系统快捷键，就直接返回，不往后面继续监测 */
    if (KGC_BarKeyDown(even)) 
        return;
    /* 发送到窗口 */
    KGC_WindowKeyDown(even);
    /* 发送到桌面 */
    KGC_DesktopKeyDown(even);
}

PUBLIC void KGC_KeyUp(KGC_KeyboardEven_t *even)
{
    /* 如果已经执行了系统快捷键，就直接返回，不往后面继续监测 */
    if (KGC_BarKeyUp(even)) 
        return;
    /* 发送到窗口 */
    
    KGC_WindowKeyUp(even);
    /* 发送到桌面 */
    KGC_DesktopKeyUp(even);
}
 