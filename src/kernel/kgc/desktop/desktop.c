/*
 * file:		kernel/kgc/desktop/desktop.c
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>
#include <video/video.h>
#include <kgc/color.h>
#include <kgc/desktop/desktop.h>
#include <kgc/container/draw.h>
#include <kgc/window/window.h>
#include <kgc/controls/label.h>
#include <kgc/bar/bar.h>
#include <lib/string.h>
#include <video/video.h>

#define KGC_DESKTOP_COLOR KGCC_ARGB(255, 60, 60, 60)

KGC_Desktop_t kgcDesktop;

/* 将输入转换成特殊的消息转发给当前窗口 */

PUBLIC void KGC_DesktopDoTimer(int ticks)
{
    /* 当前窗口为空才发送到桌面 */
    if (GET_CURRENT_WINDOW() == NULL) {
        //printk("desktop timer\n");    
    }
}

PUBLIC void KGC_DesktopMouseDown(int button, int mx, int my)
{
    /* 当前窗口为空才发送到桌面 */
    if (GET_CURRENT_WINDOW() == NULL) {
        //printk("desktop mouse down\n");    
    }
}

PUBLIC void KGC_DesktopMouseUp(int button, int mx, int my)
{
    /* 当前窗口为空才发送到桌面 */
    if (GET_CURRENT_WINDOW() == NULL) {
        //printk("desktop mouse up\n");    
    }
}

PUBLIC void KGC_DesktopMouseMotion(int mx, int my)
{
    /* 当前窗口为空才发送到桌面 */
    if (GET_CURRENT_WINDOW() == NULL) {
        //printk("desktop mouse motion\n");    
    }
}

/**
 * KGC_DesktopKeyDown - 桌面按键按下
 * @even: 按键事件
 * 
 */
PUBLIC int KGC_DesktopKeyDown(KGC_KeyboardEven_t *even)
{
    /* 当前窗口为空才发送到桌面 */
    if (GET_CURRENT_WINDOW() == NULL) {
        //printk("desktop key down\n");    
    }
    return 0;
}

PUBLIC int KGC_DesktopKeyUp(KGC_KeyboardEven_t *even)
{
    /* 当前窗口为空才发送到桌面 */
    if (GET_CURRENT_WINDOW() == NULL) {
        //printk("desktop key up\n");    
    }
    return 0;
}

PUBLIC int KGC_InitDesktopContainer()
{
    /* 桌面容器是第一个容器 */
    kgcDesktop.container = KGC_ContainerAlloc();
    if (kgcDesktop.container == NULL)
        Panic("alloc memory for container failed!\n");
    KGC_ContainerInit(kgcDesktop.container, "desktop", 0, 0,
        videoInfo.xResolution, videoInfo.yResolution);    
    KGC_ContainerDrawRectangle(kgcDesktop.container, 0, 0, 
        videoInfo.xResolution, videoInfo.yResolution, KGC_DESKTOP_COLOR);
    KGC_ContainerZ(kgcDesktop.container, 0);
    
    /* 显示桌面信息 */
    KGC_Label_t *desklabel = KGC_CreateLabel();
    char str[32];
    memset(str, 0, 32);
    strcpy(str, OS_NAME " " OS_VERSION);

    KGC_LabelSetLocation(desklabel, 0, KGC_BAR_HEIGHT + 16);
    KGC_LabelSetSize(desklabel, strlen(str) * 8, 16);
    KGC_LabelSetText(desklabel, str);
    KGC_LabelAdd(desklabel, kgcDesktop.container);
    KGC_LabelShow((KGC_Widget_t *)desklabel);
    
    return 0;
}