/*
 * file:		kernel/kgc/bar/menu_bar.c
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
#include <kgc/window/message.h>
#include <kgc/bar/menubar.h>
#include <kgc/bar/bar.h>
#include <kgc/input/mouse.h>
#include <kgc/container/draw.h>

#include <clock/clock.h>
#include <lib/vsprintf.h>
#include <lib/string.h>

EXTERN KGC_Bar_t kgcbar;

PUBLIC void KGC_MenuBarHandler(KGC_Button_t *button)
{
    
}

/** 
 * KGC_MenuBarUpdateTime - 更新时间显示
 * 
 */
PUBLIC void KGC_MenuBarUpdateTime()
{
    /* 先将原来的位置清空 */
    KGC_ContainerDrawRectangle(kgcbar.container, kgcbar.time->label.x,
        kgcbar.time->label.y, kgcbar.time->label.width, 
        kgcbar.time->label.height, KGC_MENU_BAR_COLOR);

    /* 解析时间 */
    char buf[6];
    memset(buf, 0, 6);
    sprintf(buf, "%d:%d", systemDate.hour, systemDate.minute);
    /* 设置文本，然后显示 */
    KGC_ButtonSetText(kgcbar.time, buf);
    KGC_ButtonShow((KGC_Widget_t *)kgcbar.time);
}

PUBLIC int KGC_InitMenuBar()
{
    
    kgcbar.time = KGC_CreateButton();
    if (kgcbar.time == NULL) {
        printk("create button failed!\n");
    } 
    KGC_ButtonSetLocation(kgcbar.time, kgcbar.container->width - 6*8, 0);
    KGC_ButtonSetSize(kgcbar.time, 6*8, 24);
    char buf[6];
    memset(buf, 0, 6);
    sprintf(buf, "%d:%d", systemDate.hour, systemDate.minute);
    KGC_ButtonSetText(kgcbar.time, buf);
    KGC_ButtonSetName(kgcbar.time, "time");
    KGC_ButtonAdd(kgcbar.time, kgcbar.container);
    KGC_ButtonSetHandler(kgcbar.time, &KGC_MenuBarHandler);
    KGC_ButtonShow(&kgcbar.time->label.widget);

    return 0;
}
    